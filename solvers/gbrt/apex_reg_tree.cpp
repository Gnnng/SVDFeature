/*
 *  Copyright 2011-2012 APEX Data & Knowledge Management Lab, Shanghai Jiao Tong University
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*!
 * \file apex_reg_tree.cpp
 * \brief GBRT implementation 
 * \author Tianqi Chen: tqchen@apex.sjtu.edu.cn
 */
// implementation of reg tree
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#include <climits>
#include "apex_reg_tree.h"
#include "../../apex-tensor/apex_random.h"
#include <cstring>
#include <algorithm>

namespace apex_rt{
    // whether to check bugs
    const bool check_bug = false;
    
    const float rt_eps = 1e-5f;
    const float rt_2eps = rt_eps * 2.0f;
	
	inline double sqr( double a ){
        return a * a;
    }
    
    inline void assert_sorted( unsigned *idset, int len ){
        if( !rt_debug || !check_bug ) return;
        for( int i = 1; i < len; i ++ ){
            apex_utils::assert_true( idset[i-1] < idset[i], "idset not sorted" );
        }
    }
};

namespace apex_rt{
    // struct for reg tree
    class RTree{
    public:
        // tree parameter 
        struct Param{
            // number of start root
            int num_roots;
            // total number of nodes
            int num_nodes;
            // number of sparse feature that can get accessed by mat.get_spart
            int num_group_sparse;
            // number of deleted nodes
            int num_deleted;
            // number of specific sparse feature that associated with each row
            int num_spec_sparse;
            // number of leaf items
            int num_item;
            // number of leaf weights
            int num_leaf_weight;
            // maximum depth of the tree
            int max_depth;
            // reserved part
            int reserved[ 27 ];
            Param( void ){
                num_group_sparse = 0;
                num_spec_sparse  = 0;
                num_item = 0; num_leaf_weight = 0; max_depth = 0;
                memset( reserved, 0, sizeof( reserved ) );
            }
        };
        // tree node
        struct Node{
            // pointer to parent, highest bit is used to indicate whether it's a left child or not
            int sparent;
            // pointer to left, right
            int left, right;
            // split feature index, left split or right split depends on the highest bit
            unsigned sindex;
            // feature value of split, and extra split statistics
            float split_value;
            
            inline unsigned split_index( void ) const{
                return sindex & ( (1U<<31) - 1U );
            }
            inline bool default_left( void ) const{
                return (sindex >> 31) != 0;
            } 
            inline bool is_leaf( void ) const{
                return left == -1;
            }            
            inline float leaf_value( void ) const{
                return this->split_value;
            }
            inline int parent( void ) const{
                return sparent & ( (1U << 31) - 1 );
            } 
            inline bool is_left_child( void ) const{
                return ( sparent & (1U << 31)) != 0;
            }
            inline void set_parent( int pidx, bool is_left_child = true ){
                if( is_left_child ) pidx |= (1U << 31);
                this->sparent = pidx;
            }
            inline bool is_root( void ) const{
                return sparent == -1;
            }
            inline void set_split( unsigned split_index, float split_value, bool default_left = false ){
                if( default_left ) split_index |= (1U << 31);
                this->sindex = split_index;
                this->split_value = split_value;
            }
            inline void set_leaf( float value ){
                this->split_value = value;
                this->left = this->right = -1;
            }
            inline int get_next( float value, bool is_unknown ){
                if( is_unknown ){
                    if( this->default_left() ) return left;
                    else return right;
                } 
                if( value < split_value ) return left;
                else return right;
            }
        };

        // statistics that won't go into node but can help the tree building process
        struct NodeStat{
            // loss chg caused by current split
            float loss_chg;
            // sum statistics over the childs
            float rsum, rsum_sgrad; 
            // number of child that is leaf node known up to now
            int  leaf_child_cnt;
        };
    protected:
        // vector of nodes
        std::vector<Node> nodes;
        // stats of nodes
        std::vector<NodeStat> stats;
    protected:
        // free node space, used during training process
        std::vector<int>  deleted_nodes;
        // allocate a new node, 
        // !!!!!! NOTE: may cause BUG here, nodes.resize
        inline int alloc_node( void ){
            if( param.num_deleted != 0 ){
                int nd = deleted_nodes.back();
                deleted_nodes.pop_back();
                param.num_deleted --;
                return nd;
            }
            int nd = param.num_nodes ++;
            nodes.resize( param.num_nodes );
            stats.resize( param.num_nodes );
            return nd;
        }
        // delete a tree node
        inline void delete_node( int nid ){
            apex_utils::assert_true( nid >= param.num_roots, "can not delete root");
            deleted_nodes.push_back( nid );
            nodes[ nid ].set_parent( -1 );
            param.num_deleted ++;
        }
    public:
        // change a non leaf node to a leaf node, delete its children
        inline void chg_to_leaf( int rid, float value ){
            apex_utils::assert_true( nodes[ nodes[rid].left  ].is_leaf(), "can not a non termial child");
            apex_utils::assert_true( nodes[ nodes[rid].right ].is_leaf(), "can not a non termial child");
            this->delete_node( nodes[ rid ].left ); 
            this->delete_node( nodes[ rid ].right );
            nodes[ rid ].set_leaf( value );
        }
    public:
        // parameter
        Param param;
    public:
        RTree( void ){
            param.num_nodes = 1;
            param.num_roots = 1;
            param.num_deleted = 0;
            nodes.resize( 1 );
        }
        inline Node &operator[]( int nid ){
            return nodes[ nid ];
        }
        inline NodeStat &stat( int nid ){
            return stats[ nid ];
        }
        inline void init_model( void ){
            param.num_nodes = param.num_roots;
            nodes.resize( param.num_nodes );
            stats.resize( param.num_nodes );
            for( int i = 0; i < param.num_nodes; i ++ ){
                nodes[i].set_leaf( 0.0f );
                nodes[i].set_parent( -1 );
            }
        }
        inline void load_model( FILE *fi ){
            apex_utils::assert_true( fread( &param, sizeof(Param), 1, fi ) > 0, "load RTree Param" );
            nodes.resize( param.num_nodes );
            apex_utils::assert_true( fread( &nodes[0], sizeof(Node), nodes.size(), fi ) > 0, "load RTree Node" );
            
            deleted_nodes.resize( 0 );
            for( int i = param.num_roots; i < param.num_nodes; i ++ ){
                if( nodes[i].is_root() ) deleted_nodes.push_back( i );
            }
            apex_utils::assert_true( (int)deleted_nodes.size() == param.num_deleted, "number of deleted nodes do not match" );
        }
        inline void save_model( FILE *fo ) const{
            apex_utils::assert_true( param.num_nodes == (int)nodes.size(), "BUG" );
            fwrite( &param, sizeof(Param), 1, fo );
            fwrite( &nodes[0], sizeof(Node), nodes.size(), fo );
        }
        inline void add_childs( int nid ){
            int pleft  = this->alloc_node();
            int pright = this->alloc_node();
            nodes[ nid ].left  = pleft;
            nodes[ nid ].right = pright;
            nodes[ nodes[ nid ].left  ].set_parent( nid, true );
            nodes[ nodes[ nid ].right ].set_parent( nid, false );
        }
        inline int get_depth( int nid ) const{
            int depth = 0;
            while( !nodes[ nid ].is_root() ){
                nid = nodes[ nid ].parent();
                depth ++;
            }
            return depth;
        }
        inline int num_extra_nodes( void ) const {
            return param.num_nodes - param.num_roots - param.num_deleted;
        }
    };
    
    // training parameter for reg tree
    struct RTParamTrain{
        // learning step size for a time
        float learning_rate;
        // minimum amount of weight allowed in a child
        float min_child_weight;
        // minimum amount of weight allowed to be splitted
        float min_split_weight;
        // minimum loss change required for a split
        float min_split_loss;
        // minimum number of instance allowed for a split
        int   min_split_instance;
        // minimum number of instance allowed for a split
        int   min_child_instance;
        // maximum depth of a tree
        int   max_depth;
        // split method to use
        int   split_method;
        // split temperature
        float split_temper;
        // loss type of tree
        int   loss_type;
        // weight decay parameter used to control leaf fitting
        float wd_child;
        // specific split loss for each layer
        std::vector<float> layer_split_loss;

        RTParamTrain( void ){
            learning_rate = 0.3f;
            min_child_weight = 10.0f;
            min_split_weight = 20.0f;
            min_split_loss   = 10.0f;
            min_child_instance = 100;
            min_split_instance = 500;
            max_depth = 6;
            split_method = 1;
            split_temper = 1.0f;
            wd_child = 0.0f;
            loss_type = 0;
        }
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "learning_rate") )     learning_rate = (float)atof( val );
            if( !strcmp( name, "min_child_weight") )  min_child_weight = (float)atof( val );
            if( !strcmp( name, "min_split_weight") )  min_split_weight = (float)atof( val );
            if( !strcmp( name, "min_split_loss") )    min_split_loss = (float)atof( val );
            if( !strcmp( name, "layer_split_loss") )  layer_split_loss.push_back( (float)atof( val ) );
            if( !strcmp( name, "max_depth") )         max_depth = atoi( val );
            if( !strcmp( name, "min_split_instance") )  min_split_instance = atoi( val );
            if( !strcmp( name, "min_child_instance") )  min_child_instance = atoi( val );
            if( !strcmp( name, "split_method") )        split_method = atoi( val );
            if( !strcmp( name, "split_temper") )        split_temper = (float)atof( val );
            if( !strcmp( name, "rt_loss_type") )        loss_type = atoi( val );
            if( !strcmp( name, "wd_child") )            wd_child = (float)atof( val );
        }
        inline float get_min_split_loss( int depth ) const{
            return depth < (int)layer_split_loss.size() ? layer_split_loss[ depth ] : min_split_loss;
        }
    };
    
    // selecter of rtree to find the suitable candidate
    class RTSelecter{
    public:
        struct Entry{
            float  loss_chg;
            size_t start;
            int    len;
            unsigned sindex;
            float    split_value;
            Entry(){}
            Entry( float loss_chg, size_t start, int len, unsigned split_index, float split_value, bool default_left ){
                this->loss_chg = loss_chg;
                this->start    = start;
                this->len      = len;
                if( default_left ) split_index |= (1U << 31);
                this->sindex = split_index;
                this->split_value = split_value;
            }
            inline unsigned split_index( void ) const{
                return sindex & ( (1U<<31) - 1U );
            }
            inline bool default_left( void ) const{
                return (sindex >> 31) != 0;
            }             
        };
    private:
        Entry best_entry;
        std::vector<Entry> entrys;
        const RTParamTrain &param;
    private:
        inline static int random_select( const std::vector<float> &w, float s_max ){
            float rnd = static_cast<float>( apex_random::next_double() * s_max );
            size_t idx = std::lower_bound( w.begin(), w.end(), rnd ) - w.begin();
            if( idx >= w.size() ) idx -= 1;
            return idx;
        }
        // select entry by probability
        inline const Entry &select_prob( void ){
            double wsum = 0.0;
            const float beta = 1.0f / param.split_temper;
            std::vector<float> wt;
            wt.resize( entrys.size() );
            for( size_t i = 0; i < entrys.size(); i ++ ){
                wsum += expf( (entrys[i].loss_chg - best_entry.loss_chg) * beta );
                wt[ i ] = wsum;
            }
            if( entrys.size() == 0 ) return best_entry;
            return entrys[ random_select( wt, wsum ) ];
        }
    public:
        RTSelecter( const RTParamTrain &p ):param( p ){
            memset( &best_entry, 0, sizeof(best_entry) );
            best_entry.loss_chg = 0.0f;
        }
        inline void push_back( const Entry &e, float min_split_loss ){
            // different split method are different from each other
            switch( param.split_method ){                
            case 0:{
                // prune in selection phase
                if( e.loss_chg < min_split_loss ) return;
            }
            case 1:{
                // simply get the best, leave prunning after wards
                if( e.loss_chg > best_entry.loss_chg ) best_entry = e;
                break;
            }
            case 2:{
                if( e.loss_chg > best_entry.loss_chg ) best_entry.loss_chg = e.loss_chg;
                entrys.push_back( e );
                break;
            }
            default: apex_utils::error("unknown split method");
            }
        }
        inline const Entry & select( void ){            
            switch( param.split_method ){                
            case 0:
            case 1: return best_entry;
            case 2: return select_prob();
            default: apex_utils::error("unknown split method"); return best_entry;
            }
        }
    };

    // updater of rtree, allows the parameters to be stored inside, key solver
    class RTreeUpdater{
    private:
        // training task
        struct Task{
            // node id in tree
            int nid;
            // idset pointer
            unsigned *idset;
            // length of idset
            unsigned len;
            Task(){}
            Task( int nid, unsigned *idset, unsigned len ){
                this->nid = nid;
                this->idset = idset;
                this->len = len;
            }
        };

        // sparse entry
        struct SEntry{
            // feature index 
            unsigned findex;
            // feature value 
            float    fvalue;
            // row index in grad
            unsigned rindex;
            SEntry(){}
            SEntry( unsigned findex, float fvalue, unsigned rindex ){
                this->findex = findex; this->fvalue = fvalue; this->rindex = rindex;
            }
            inline bool operator<( const SEntry &p ) const{
                if( findex < p.findex ) return true;
                if( findex > p.findex ) return false;
                return fvalue < p.fvalue;
            }
        };
    private:
        const RTParamTrain &param;
        // parameters 
        RTree &tree;
        std::vector<float> &grad;
        std::vector<float> &grad_second;
        FMatrix &mat;
        std::vector<unsigned> &group_id;
        std::vector<float>    &weight;
        FMatrixS &smat;
    private:
        // maximum depth up to now
        int max_depth;
        // number of nodes being pruned
        int num_pruned;
        // stack to store current task
        std::vector<Task> task_stack;
        // temporal space for index set
        std::vector<unsigned> idset;
    private:
        inline void add_task( Task tsk ){
            task_stack.push_back( tsk );
        }
        inline bool next_task( Task &tsk ){
            if( task_stack.size() == 0 ) return false;
            tsk = task_stack.back(); 
            task_stack.pop_back();
            return true;
        } 
    private:
        // get weight for current instance
        inline float get_weight( unsigned ridx ){
            if( param.loss_type == 0 ){
                return weight.size() == 0 ? 1.0f : weight[ ridx ];
            }else{
                // for compability issue
                return grad_second[ ridx ] * 4.0f;
            }
        }
    private:
        // try to prune off current leaf, return true if successful
        inline void try_prune_leaf( int nid, double rsum, double rsum_sgrad, int depth ){
            if( tree[ nid ].is_root() ) return;
            int pid = tree[ nid ].parent();
            RTree::NodeStat &s = tree.stat( pid );
            s.leaf_child_cnt ++; 
            s.rsum = s.rsum + rsum; 
            s.rsum_sgrad = s.rsum_sgrad + rsum_sgrad;

            if( s.leaf_child_cnt >= 2 && s.loss_chg < param.get_min_split_loss( depth - 1 ) ){
                // need to be pruned
                apex_utils::assert_true( rsum_sgrad > 1e-5f, "second order derivative too low" );
                tree.chg_to_leaf( pid, - param.learning_rate * s.rsum /( s.rsum_sgrad + param.wd_child ) );
                // add statistics to number of nodes pruned
                num_pruned += 2;
                // tail recursion
                this->try_prune_leaf( pid, s.rsum, s.rsum_sgrad, depth - 1 );
            }
        }
        // make leaf for current node :)
        inline void make_leaf( Task tsk, double rsum, double rweight, bool compute ){
            double rsum_sgrad = 0.0;
            for( unsigned i = 0; i < tsk.len; i ++ ){
                const unsigned ridx = tsk.idset[i];
                const float sgd = grad_second[ ridx ];
                rsum_sgrad += sgd;
                if( compute ){
                    rsum += grad[ ridx ];
                    rweight += this->get_weight( ridx );
                }
            }
            if( rweight < param.min_child_weight ){
                tree[ tsk.nid ].set_leaf( 0.0f );
            }else{                
                apex_utils::assert_true( rsum_sgrad > 1e-5f, "second order derivative too low" );
                tree[ tsk.nid ].set_leaf( - param.learning_rate * rsum / ( rsum_sgrad + param.wd_child ) );                
            }
            this->try_prune_leaf( tsk.nid, rsum, rsum_sgrad, tree.get_depth( tsk.nid ) );
        }
        
        // make split for current task, re-arrange positions in idset
        inline void make_split( Task tsk, SEntry *entry, int num, float loss_chg ){
            // before split, first prepare statistics
            RTree::NodeStat &s = tree.stat( tsk.nid );
            s.loss_chg = loss_chg; 
            s.leaf_child_cnt = 0;
            s.rsum = s.rsum_sgrad = 0.0f;
            // add childs to current node
            tree.add_childs( tsk.nid );
            // assert that idset is sorted
            assert_sorted( tsk.idset, tsk.len );
            // use merge sort style to get the solution
            std::vector<unsigned> qset;
            for( int i = 0; i < num; i ++ ){
                qset.push_back( entry[i].rindex );
            }
            std::sort( qset.begin(), qset.end() );            
            // do merge sort style, make the left set
            for( unsigned i = 0, top = 0; i < tsk.len; i ++ ){
                if( top < qset.size() ){
                    if( tsk.idset[ i ] != qset[ top ] ){
                        tsk.idset[ i - top ] = tsk.idset[ i ];
                    }else{
                        top ++;
                    }
                }else{
                    tsk.idset[ i - qset.size() ] = tsk.idset[ i ];
                }
            }
            // get two parts 
            RTree::Node &n = tree[ tsk.nid ];
            Task def_part( n.default_left() ? n.left : n.right, tsk.idset, tsk.len - qset.size() );
            Task spl_part( n.default_left() ? n.right: n.left , tsk.idset + def_part.len, qset.size() );  
            // fill back split part
            for( unsigned i = 0; i < spl_part.len; i ++ ){
                spl_part.idset[ i ] = qset[ i ];
            }
            // add tasks to the queue
            this->add_task( def_part ); 
            this->add_task( spl_part );
        }

        // find split for current task, compare all instances once
        inline void expand( Task tsk ){
            // assert that idset is sorted
            // if reach maximum depth, make leaf from current node
            int depth = tree.get_depth( tsk.nid );
            if( depth > max_depth ) max_depth = depth; 
            if( depth >= param.max_depth || tsk.len < (unsigned)param.min_split_instance ){
                this->make_leaf( tsk, 0.0, 0.0, true ); return;
            } 

            // get min split loss
            const float min_split_loss = param.get_min_split_loss( depth );

            std::vector<SEntry> entry;
            // statistics of root
            double rsum = 0.0, rweight = 0.0;
            for( unsigned i = 0; i < tsk.len; i ++ ){
                const unsigned ridx = tsk.idset[i];
                rsum     += grad[ ridx ];
                rweight  += this->get_weight( ridx );;
                {// add sparse part
                    FVectorSparse sp = mat.get_spart( ridx );
                    for( int j = 0; j < sp.len; j ++ ){
                        entry.push_back( SEntry( sp.findex[j], sp.fvalue[j], ridx ) );
                    }
                }
                {// add extra sparse part, if any
                    if( ridx < smat.num_row() ){
                        const int base = tree.param.num_group_sparse;
                        FVectorSparse sp = smat[ ridx ];
                        for( int j = 0; j < sp.len; j ++ ){
                            entry.push_back( SEntry( sp.findex[j] + base, sp.fvalue[j], ridx ) );
                        }
                    }
                }
                {// add dense part, brute force way -_-
                    FVector v = mat[ ridx ];
                    const int base = tree.param.num_group_sparse + tree.param.num_spec_sparse;
                    for( int j = 0; j < v.size(); j ++ ){
                        if( !v.is_unknown( j ) ){
                            entry.push_back( SEntry( j + base, v[j], ridx ) );
                        }
                    }
                }
            }

            // if minimum split weight is not meet
            if( rweight < param.min_split_weight ){
                this->make_leaf( tsk, rsum, rweight, false ); return; 
            }

            // global selecter
            RTSelecter sglobal( param );
            
            const double rmean_sqr_sum = sqr( rsum / rweight ) * rweight;

            // sort and start to emumerate over the splits
            std::sort( entry.begin(), entry.end() );
            
            for( size_t i = 0; i < entry.size(); ){
                size_t top = i + 1;
                while( top < entry.size() && entry[ top ].findex == entry[i].findex ) top ++;
                // local selecter
                RTSelecter slocal( param );

                {// forward process, default right
                    double csum = 0.0, cweight = 0.0;
                    for( size_t j = i; j < top; j ++ ){
                        const unsigned ridx = entry[ j ].rindex;
                        csum     += grad[ ridx ];
                        cweight  += this->get_weight( ridx );
                        // check for split
                        if( j == top - 1 || entry[j].fvalue + rt_2eps < entry[ j + 1 ].fvalue ){
                            const int clen = static_cast<int>( j + 1 - i );
                            if( clen < param.min_child_instance || cweight < param.min_child_weight ) continue;
                            const int dlen = static_cast<int>( tsk.len - clen );
                            const double dweight = rweight - cweight;
                            if( dlen < param.min_child_instance || dweight < param.min_child_weight ) break;

                            double loss_chg = sqr( csum / cweight ) * cweight + sqr( (rsum - csum) / dweight ) * dweight - rmean_sqr_sum;
                            // add candidate to selecter
                            slocal.push_back( RTSelecter::Entry( loss_chg, i, clen, entry[j].findex, 
                                                                 j == top-1 ? entry[j].fvalue + rt_eps :0.5 * (entry[j].fvalue+entry[j+1].fvalue),
                                                                 false ), min_split_loss );
                        }
                    }
                }
                {// backward process, default left
                    double csum = 0.0, cweight = 0.0;                    
                    for( size_t j = top; j > i; j -- ){
                        const unsigned ridx = entry[ j - 1 ].rindex;
                        csum     += grad[ ridx ];
                        cweight  += this->get_weight( ridx );
                        // check for split
                        if( j == i + 1 || entry[ j - 2 ].fvalue + rt_2eps < entry[ j - 1 ].fvalue ){
                            const int clen = static_cast<int>( top - j + 1 );
                            if( clen < param.min_child_instance || cweight < param.min_child_weight ) continue;
                            const int dlen = static_cast<int>( tsk.len - clen );
                            const double dweight = rweight - cweight;
                            if( dlen < param.min_child_instance || dweight < param.min_child_weight ) break;
                            double loss_chg = sqr( csum / cweight ) * cweight + sqr( (rsum - csum) / dweight ) * dweight - rmean_sqr_sum;
                            // add candidate to selecter                            
                            slocal.push_back( RTSelecter::Entry( loss_chg, j-1, clen, entry[j-1].findex, 
                                                                 j == i + 1 ? entry[j-1].fvalue - rt_eps : 0.5 * (entry[j-2].fvalue + entry[j-1].fvalue), 
                                                                 true ), min_split_loss );
                        }
                    }
                    sglobal.push_back( slocal.select(), min_split_loss );
                }
                // make i to be top
                i = top;
            }
            
            const RTSelecter::Entry &e = sglobal.select();
            // allowed to split
            if( e.loss_chg > rt_eps ){
                // add splits
                tree[ tsk.nid ].set_split( e.split_index(), e.split_value, e.default_left() );
                this->make_split( tsk, &entry[ e.start ], e.len, e.loss_chg ); 
            }else{
                // make leaf if we didn't meet requirement
                this->make_leaf( tsk, rsum, rweight, false );
            }
        }
    private:
        // initialize the tasks
        inline void init_tasks( size_t ngrads ){
            idset.resize( ngrads );            
            // add group partition if necessary
            if( group_id.size() == 0 ){
                for( size_t i = 0; i < ngrads; i ++ ){
                    idset[i] = (unsigned)i;
                } 
                this->add_task( Task( 0, &idset[0], idset.size() ) ); return;
            }
            apex_utils::assert_true( group_id.size() == ngrads, "number of groups must be exact" );
            // partition by group
            std::vector< std::pair< unsigned, unsigned > > gps;
            gps.reserve( ngrads );
            for( size_t i = 0; i < ngrads; i ++ ){
                apex_utils::assert_true( group_id[ i ] < (unsigned)tree.param.num_roots, "group id exceed number of roots" );
                gps.push_back( std::make_pair( group_id[i], static_cast<unsigned>(i) ) );
            } 
            // sort then cut
            std::sort( gps.begin(), gps.end() );
            for( size_t i = 0; i < ngrads;){
                size_t j = i;
                while( j < ngrads && gps[j].first == gps[i].first ){
                    idset[j] = gps[j].second; ++ j;
                }
                this->add_task( Task( gps[i].first, &idset[i], j - i ) );
                i = j;
            }
        }
    public:
        RTreeUpdater( const RTParamTrain &pparam, 
                      RTree &ptree,
                      std::vector<float> &pgrad,
                      std::vector<float> &pgrad_second,
                      FMatrix &pmat,
                      std::vector<unsigned> &pgroup_id,
                      std::vector<float>    &pweight,
                      FMatrixS &psmat ):
            param( pparam ), tree( ptree ), grad( pgrad ), grad_second( pgrad_second ),
            mat( pmat ), group_id( pgroup_id ), weight( pweight ), smat( psmat ){            
        }
        inline int do_boost( int &num_pruned ){
            this->init_tasks( grad.size() );
            this->max_depth = 0;
            this->num_pruned = 0;
            Task tsk;
            while( this->next_task( tsk ) ){
                this->expand( tsk );
            }
            num_pruned = this->num_pruned;
            return max_depth;
        }
    };
    
    class RTreeTrainer : public IRTTrainer{
    private:
        int silent;
        // tree of current shape 
        RTree tree;
        RTParamTrain param;
    public:
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "silent") )  silent = atoi( val );
            if( !strcmp( name, "rt_num_group") )  tree.param.num_roots = atoi( val );
            if( !strcmp( name, "rt_num_group_sparse") ) tree.param.num_group_sparse = atoi( val );
            if( !strcmp( name, "rt_num_spec_sparse") )  tree.param.num_spec_sparse  = atoi( val );
            param.set_param( name, val );
        }
        virtual void load_model( FILE *fi ){
            tree.load_model( fi );
        }
        virtual void save_model( FILE *fo ) const{
            tree.save_model( fo );
        }
        virtual void init_trainer( void ){
            tree.init_model();
        }
    public:
        virtual void do_boost( std::vector<float> &grad, 
                               std::vector<float> &grad_second,
                               FMatrix &mat,
                               std::vector<unsigned> &group_id,
                               std::vector<float>    &weight,
                               FMatrixS &smat ){
            apex_utils::assert_true( grad.size() < UINT_MAX, "number of instance exceed what we can handle" );
            if( check_bug || !silent ){
                printf( "\nbuild GBRT with %u instances\n", (unsigned)grad.size() );
            }
            // start with a id set
            RTreeUpdater updater( param, tree, grad, grad_second, mat, group_id, weight, smat );
            int num_pruned;
            tree.param.max_depth = updater.do_boost( num_pruned );

            if( check_bug || !silent ){
                printf( "tree train end, %d roots, %d extra nodes, %d pruned nodes ,max_depth=%d\n", 
                        tree.param.num_roots, tree.num_extra_nodes(), num_pruned, tree.param.max_depth );
            }
        }

        virtual int get_leaf_id( const FVector &feat, const FVector &fcommon, unsigned gid = 0 ){
            // start from groups that belongs to current data
            int pid = (int)gid;
            // tranverse tree
            while( !tree[ pid ].is_leaf() ){
                unsigned split_index = tree[ pid ].split_index();
                if( split_index < (unsigned)fcommon.size() ){
                    pid = tree[ pid ].get_next( fcommon[ split_index ], fcommon.is_unknown( split_index ) );
                } else{
                    split_index -= fcommon.size();
                    apex_utils::assert_true( split_index < (unsigned)feat.size(), "split index not found in input feature" );
                    pid = tree[ pid ].get_next( feat[ split_index ], feat.is_unknown( split_index ) );
                }
            }
            return pid;
        }

        virtual float predict( const FVector &feat, const FVector &fcommon, unsigned gid = 0 ){
            // start from groups that belongs to current data
            int pid = RTreeTrainer::get_leaf_id( feat, fcommon, gid );
            return tree[ pid ].leaf_value();
        }
        
        virtual void get_stats( int &max_depth, int &num_extra_nodes ) const{
            max_depth = tree.param.max_depth; 
            num_extra_nodes = tree.num_extra_nodes();
        }
    public:
        RTreeTrainer( void ){ silent = 0; }
        virtual ~RTreeTrainer( void ){}
    };
};

namespace apex_rt{
    IRTTrainer *create_rt_trainer( int rt_type ){
        return new RTreeTrainer();
    }
};
