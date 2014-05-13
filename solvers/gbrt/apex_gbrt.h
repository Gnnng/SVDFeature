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
 * \file apex_reg_tree.h
 * \brief SVDFeature wrapper to the GBRT
 * \author Tianqi Chen: tqchen@apex.sjtu.edu.cn
 */

#ifndef _APEX_GBRT_H_
#define _APEX_GBRT_H_

#include "apex_reg_tree.h"
#include "../../apex_svd.h"
#include "../../apex_svd_model.h"
#include "../../apex-utils/apex_config.h"
#include <cstring>
#include <vector>

// GBRT implementation of SVDFeature
namespace apex_svd{
    // extra training param for GBRT
    struct GBRTTrainParam{
    private:
        // inner round counter
        int round_counter;
    public:
        /*! \brief learning rate */
        float learning_rate;
        /*! 
         * \brief whether to decay learning rate during training, default=0 
         * \sa decay_rate
         */
        int decay_learning_rate;
        /*! 
         * \brief learning rate decay rate, only valid when decay_learning_rate=1
         *   for every round, learning_rate will be timed by decay_rate
         */
        float decay_rate;       
        /*! 
         * \brief min learning rate allowed during decay
         */        
        float min_learning_rate;

        GBRTTrainParam( void ){
            round_counter = 0;
            learning_rate = 0.01f;
            decay_learning_rate = 0;
            decay_rate = 1.0f;
            min_learning_rate = 0.001f;
        }
        inline void set_param( const char *name, const char *val ){
            if( !strcmp("learning_rate" , name ) ) learning_rate = (float)atof( val );
            if( !strcmp("decay_learning_rate" , name ) )  decay_learning_rate = atoi( val );
            if( !strcmp("min_learning_rate" , name ) )    min_learning_rate = (float)atof( val );
            if( !strcmp("decay_rate" , name ) )           decay_rate = (float)atof( val );
        }
        inline void set_round( int nround ){
            if( decay_learning_rate != 0 ){
                apex_utils::assert_true( round_counter <= nround, "round counter restriction" );
                while( round_counter < nround ){                    
                    learning_rate *= decay_rate;
                    round_counter ++;
                }
                if( learning_rate < min_learning_rate ) learning_rate = min_learning_rate; 
            }
        }        
    };

    // model parameter for GBRT
    struct GBRTModelParam{
        // number of trees
        int num_trees;
        // configure of baseline predictor
        int baseline_mode;
        // type of tree used
        int tree_type;
        // number of item feature
        int num_item;
        // number of global feature
        int num_global;
        // number of user feedback information
        int num_ufeedback;
        // number of spec sparse instance
        int num_spec_sparse;
        // number of derived root type, default 0 equals 1
        int use_tax_root;
        // the switch of how to make use of item feature
        int item_feature_mode;
        // number of root weight that is passed through global feature
        int num_root_weight;
        // reserved parameters
        int reserved[ 28 ];
        // constructor
        GBRTModelParam( void ){
            num_trees = 0; 
            baseline_mode = 0; 
            tree_type = 0;
            num_item = num_global = num_ufeedback = num_spec_sparse = 0;
            use_tax_root = 0; item_feature_mode = 0; 
            num_root_weight = 0;
            memset( reserved, 0, sizeof( reserved ) );            
        }
        inline void set_param( const char *name, const char *val ){
            if( !strcmp("rt_baseline", name ) ) baseline_mode = atoi( val );
            if( !strcmp("rt_type", name ) ) tree_type = atoi( val );
            if( !strcmp("num_item", name ) )      num_item = atoi( val );
            if( !strcmp("num_global", name ) )    num_global = atoi( val );
            if( !strcmp("num_ufeedback", name ) ) num_ufeedback = atoi( val );
            if( !strcmp("num_spec_sparse", name ) ) num_spec_sparse = atoi( val );
            if( !strcmp("use_tax_root", name ) )   use_tax_root = atoi( val );
            if( !strcmp("item_feature_mode", name ) ) item_feature_mode = atoi( val );
            if( !strcmp("num_root_weight", name ) )   num_root_weight = atoi( val );            
        }
    };
    
    struct GBRTModel{
        /*! \brief type of the solver */
        SVDTypeParam  mtype;
        /*! \brief model parameters */ 
        GBRTModelParam param;
        /*! \brief component trees */ 
        std::vector<apex_rt::IRTTrainer*> trees;
        /*! \brief type of root */ 
        std::vector<int> root_type;
        /*! \brief type of root weight */ 
        std::vector<int> weight_type;
        /*! \brief free space of the model */
        inline void free_space( void ){
            for( size_t i = 0; i < trees.size(); i ++ ){
                delete trees[i];
            }
            trees.clear(); param.num_trees = 0; 
            root_type.clear(); weight_type.clear();
        }
        inline void load_from_file( FILE *fi ){
            if( trees.size() != 0 ) this->free_space();
            if( fread( &param, sizeof(GBRTModelParam) , 1 , fi ) == 0 ){
                printf("error loading GBRT model\n"); exit( -1 );
            }
            trees.resize( param.num_trees );
            for( size_t i = 0; i < trees.size(); i ++ ){
                trees[ i ] = apex_rt::create_rt_trainer( param.tree_type );
                trees[ i ]->load_model( fi );
            }
            if( param.use_tax_root != 0 ){
                root_type.resize( param.num_trees );
                if( param.num_trees != 0 ){
                    apex_utils::assert_true( fread( &root_type[0], sizeof(int), param.num_trees, fi ) > 0, "load root type" );
                }
            }
            if( param.num_root_weight != 0 ){
                weight_type.resize( param.num_trees );
                if( param.num_trees != 0 ){
                    apex_utils::assert_true( fread( &weight_type[0], sizeof(int), param.num_trees, fi ) > 0, "load weight type" );
                }
            }     
        } 
        inline void save_to_file( FILE *fo ) const{
            apex_utils::assert_true( param.num_trees == (int)trees.size(), "bug: GBRT model inconsistent");
            fwrite( &param, sizeof(GBRTModelParam) , 1 , fo );
            for( size_t i = 0; i < trees.size(); i ++ ){
                trees[ i ]->save_model( fo ); 
            }
            if( param.use_tax_root != 0 && param.num_trees != 0 ){
                fwrite( &root_type[0], sizeof(int), param.num_trees, fo );
            }
            if( param.num_root_weight != 0 && param.num_trees != 0 ){
                fwrite( &weight_type[0], sizeof(int), param.num_trees, fo );
            }
        }
        inline void push_back( apex_rt::IRTTrainer *tptr, int rtype = -1, int wtype = -1 ){
            trees.push_back( tptr ); param.num_trees ++;
            if( param.use_tax_root != 0 ){
                apex_utils::assert_true( rtype >=0, "must specify root type for each tree");
                root_type.push_back( rtype );
            }
            if( param.num_root_weight != 0 ){
                apex_utils::assert_true( wtype >=0, "must specify weight type for each tree");
                weight_type.push_back( wtype );
            }            
        }
        inline void check_init( void ) const{
            apex_utils::assert_true( param.num_trees == (int)trees.size(), "bug: GBRT model inconsistent");
            apex_utils::assert_true( param.num_trees == 0, "bug: GBRT model inconsistent");
            if( param.use_tax_root != 0 ){
                apex_utils::assert_true( root_type.size() == 0, "bug: GBRT model inconsistent");
            }
            if( param.num_root_weight != 0 ){
                apex_utils::assert_true( weight_type.size() == 0, "bug: GBRT model inconsistent");
            }            
        }
    };
};

namespace apex_svd{
    // data structure to store item taxonomy
    class ItemTaxonomy{
    private:
        int num_item, num_label;
        std::vector<int> sizes;
        std::vector<int> data;
    public:
        ItemTaxonomy( void ){
            num_item = 0; num_label = 0;            
        }
        inline void load( const char *fname ){
            FILE *fi = apex_utils::fopen_check( fname, "r" );
            apex_utils::assert_true( fscanf( fi, "%d%d", &num_item, &num_label ) == 2, "load tax" );
            sizes.resize( num_label );
            for( int i = 0; i < num_label; i ++ ){
                apex_utils::assert_true( fscanf( fi, "%d", &sizes[i] ) == 1, "load tax" );
            }
            data.resize( num_item * num_label );
            for( int i = 0; i < num_item; i ++ ){
                for( int j = 0; j < num_label; j ++ ){                    
                    apex_utils::assert_true( fscanf( fi, "%d", &data[i*num_label+j] ) == 1, "load tax" );
                    apex_utils::assert_true( data[i*num_label+j] < sizes[j] , "load tax" );
                }
            }
            fclose( fi );
        }
        inline int size( int rtype ) const{
            apex_utils::assert_true( !apex_rt::rt_debug || rtype < num_label, "rtype exceed bound"); 
            return sizes[ rtype ];
        }
        inline int nlabel( void ) const{
            return num_label;
        }
        inline const int* operator[]( int iid ) const{
            apex_utils::assert_true( !apex_rt::rt_debug || iid < num_item, "iid exceed bound");             
            return &data[ iid * num_label ];
        }
    };

    // scheduler that make plans for which type of setting to choose
    class GBRTScheduler{
    private:
        // current root type
        int type_current;
        // chg mode of rtype
        int type_chg_cycle;
        // chg mode of rtype
        int type_start_default;
        // chg mode of rtype
        int type_start_cycle;
        // default rtype
        int type_default;
        // specific set type for each round within a cycle
        std::vector<int> type_set;
        // specific set type for specific type
        std::vector<int> type_round;
        // length of prefix
        int  plen;
        // prefix character
        char prefix[ 256 ];
    private:
        // the round to start random type choosing
        int type_start_random;
        // choosing weight of each type
        std::vector<float> type_weight;
        // function to do random choose
        inline int random_choose_idx( void ){
            apex_utils::assert_true( type_weight.size() > 0, "must have specific typew" );
            std::vector<float> wt;
            double sum = 0.0;
            for( size_t i = 0; i < type_weight.size(); i ++ ){
                wt.push_back( sum += type_weight[i] );
            }
            double rnd = apex_random::next_double() * sum;
            size_t idx = std::lower_bound( wt.begin(), wt.end(), rnd ) - wt.begin();
            if( idx >= wt.size() ) idx = wt.size() - 1;
            return (int)idx;            
        }
    public:
        GBRTScheduler( const char *prefix ){
            strcpy( this->prefix, prefix );
            plen = strlen( prefix );
            type_current = 0; 
            type_default = 0;
            type_chg_cycle = 1; 
            type_start_cycle = 0;
            type_start_default = INT_MAX; 
            type_start_random = INT_MAX;
        }
        inline void set_round( int nround ){
            if( nround < (int)type_round.size() && type_round[ nround ] != -1 ){
                this->type_current = type_round[ nround ]; return;
            }
            if( nround >= type_start_default || nround < type_start_cycle ){
                this->type_current = type_default; return;
            }            
            int idx = nround % type_chg_cycle;
            if( nround >= type_start_random ){
                idx = this->random_choose_idx();
            }
            if( idx < (int)type_set.size() ){
                this->type_current = type_set[ idx ];
            }else{
                this->type_current = type_default;
            }
        }
        inline void set_param( const char *name, const char *val ){
            if( strncmp( name, prefix, plen ) ) return;
            name += plen;
            if( !strcmp( name, "type_chg_cycle") )    type_chg_cycle = atoi( val ); 
            if( !strcmp( name, "type_start_cycle") )  type_start_cycle = atoi( val ); 
            if( !strcmp( name, "type_start_default") )type_start_default = atoi( val ); 
            if( !strcmp( name, "type_start_random") ) type_start_random = atoi( val ); 
            if( !strcmp( name, "type_default") )      type_default   = atoi( val ); 
            
            if( !strncmp( name, "type[", 5 ) ){
                int id, start, end;
                // plugin for other nodes..
                if( sscanf( name, "type[%d-%d)", &start, &end ) == 2 && strcmp( val, "same") == 0 ){
                    while( type_set.size() < (size_t)end ){
                        type_set.push_back( type_default );
                    }
                    for( int i = start; i < end; i ++ ){
                        type_set[ i ] = i;
                    }
                    return;
                }

                apex_utils::assert_true( sscanf( name, "type[%d]", &id ) == 1, "unknown type id" );
                while( type_set.size() <= (size_t)id ){
                    type_set.push_back( type_default );
                }
                type_set[ id ] = atoi( val );
            }

            
            // force type at specific round
            if( !strncmp( name, "typef[", 6 ) ){
                int start, end;
                if( sscanf( name, "typef[%d-%d)", &start, &end ) != 2 ){
                    apex_utils::assert_true( sscanf( name, "typef[%d]", &start ) == 1, "unknown type id" );
                    end = start + 1;
                }
                while( type_round.size() < (size_t)end ){
                    type_round.push_back( -1 );
                }
                int tp = atoi( val );
                for( int i = start; i < end; i ++ ){
                    type_round[ i ] = tp;
                }
            }
            // set probability weight for each type
            if( !strncmp( name, "typew[", 6 ) ){
                int start, end;
                if( sscanf( name, "typew[%d-%d)", &start, &end ) != 2 ){
                    apex_utils::assert_true( sscanf( name, "typew[%d]", &start ) == 1, "unknown type id" );
                    end = start + 1;
                }
                while( type_weight.size() < (size_t)end ){
                    type_weight.push_back( 1.0f );
                }
                float tp = (float)atof( val );
                for( int i = start; i < end; i ++ ){
                    type_weight[ i ] = tp;
                }
            }
        }
        inline int curr_type( void ) const{
            return type_current;
        }                
    };

    // parameter scheduler for GBRT training
    class GBRTParamScheduler{
    public:
        struct Entry{
            unsigned gstart, gend;
            unsigned fstart, fend;
        };
    private:
        std::vector<Entry> entry;        
        GBRTScheduler pscheduler;
    public:
        GBRTParamScheduler( void ):
            pscheduler("p"){
            Entry e;
            e.fstart = e.gstart = 0; e.gend = e.fend = UINT_MAX;
            entry.push_back( e );
        }
        inline void set_round( int nround ){
            pscheduler.set_round( nround );
        }
        inline void set_param( const char *name, const char *val ){
            pscheduler.set_param( name, val );
            if( !strcmp( name, "pset" ) ){
                Entry e;
                apex_utils::assert_true( sscanf( val, "%u-%u.%u-%u", &e.fstart, &e.fend, &e.gstart, &e.gend ) == 4, 
                                         "error loading pset");
                entry.push_back( e );
            }
        }
        inline const Entry& curr_type( void ) const{
            return entry[ pscheduler.curr_type() ];
        }
    };
    
    // result buffer to temporally store results produced by gbrt
    template<typename SType>
    class GBRTResultBuffer{
    private:
        size_t dindex;
        bool first_round;
        std::vector<SType> buf;
    public:
        GBRTResultBuffer( void ){ 
            dindex = 0; first_round = true; 
        }
        // move cursor before first
        inline void finish_round( void ){
            dindex = 0;
            first_round = false;
        }
        // move cursor to next 
        inline void next( void ){
            dindex ++;
            if( dindex > buf.size() ){
                apex_utils::assert_true( this->is_first_round(), "can't change buffer size after first round" );
                buf.push_back( 0.0 );
            }
        }
        inline bool is_first_round( void ) const{
            return first_round;
        }
        // current storage
        inline SType &curr( void ){
            apex_utils::assert_true( dindex != 0, "need to call next" );
            return buf[ dindex - 1 ];
        }
    };

    // rank net implementation
    class GBRTTrainer: public ISVDTrainer{
    protected:
        GBRTModel model;
        GBRTTrainParam param;
    private:
        // item taxonomy 
        ItemTaxonomy tax;
        char tax_name[ 256 ];
    private:
        // saves all the configures
        apex_utils::ConfigSaver cfg;
        // instead of predicting 
        int pred_tree_leaf;
        // index of sparse part
        int spart_index;
        // type of loss function
        int rt_loss_type;
        // chg baseline mode
        int chg_baseline_mode;
    private:
        // training data result buffer
        GBRTResultBuffer<double> res_buf_train;
        // option to use result buffer
        int use_res_buf;        
    private:
        // base score
        float base_score;
        // scaling parameter over baseline
        float scale_baseline;
    protected:
        // tmp data that needed to be written by update stats
        std::vector<float> tmp_pred, tmp_grad, tmp_sgrad, tmp_weight;
    private:        
        apex_rt::FVector fcommon, feat;
        std::vector<float> fcommon_s, feat_s;
    private:        
        // data for Reg tree training
        std::vector<float> dgrad;
        std::vector<float> dgrad_second;
        apex_rt::FMatrix   dmat;
        std::vector<unsigned> dgroup_id;
        std::vector<float>    dweight;
        apex_rt::FMatrixS  dsmat;
    private:
        // root type scheduler
        GBRTScheduler rscheduler;
        // weight type scheduler
        GBRTScheduler wscheduler;
        // parameter bound scheduler
        GBRTParamScheduler pscheduler;
    protected:
        GBRTTrainer( const SVDTypeParam &mtype )
            :rscheduler("r"), wscheduler("w"){
            model.mtype = mtype;
            this->chg_baseline_mode = -1;
            this->rt_loss_type = 1;
            strcpy( tax_name, "NULL" );
            // param setting
            this->scale_baseline = 1.0f;
            // whether to predict the leaf node
            this->pred_tree_leaf = -1;
            // whether to use result buffer
            this->use_res_buf = 0;
            this->base_score = 0.0f;
        }
        virtual ~GBRTTrainer( void ){
            model.free_space();
        }
    public:
        // model related interface
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "rt_loss_type") )       rt_loss_type = atoi( val ); 
            if( !strcmp( name, "pred_tree_leaf") )     pred_tree_leaf = atoi( val ); 
            if( !strcmp( name, "chg_baseline_mode") )  chg_baseline_mode = atoi( val ); 
            if( !strcmp( name, "feature_item") )       strcpy( tax_name, val );
            if( !strcmp( name, "scale_baseline") ) scale_baseline = (float)atof( val ); 
            if( !strcmp( name, "base_score") )     base_score = (float)atof( val ); 
            if( !strcmp( "use_res_buf", name ) ) use_res_buf = atoi( val );
            if( model.param.num_trees == 0 ) model.param.set_param( name, val );
            pscheduler.set_param( name, val );
            rscheduler.set_param( name, val );
            wscheduler.set_param( name, val );
            cfg.push_back( name, val );
            param.set_param( name, val );
        }
        // load model from file
        virtual void load_model( FILE *fi ) {
            model.load_from_file( fi );
            // chg baseline
            if( chg_baseline_mode >= 0 ){
                model.param.baseline_mode = chg_baseline_mode;
            }
        }
        // save model to file
        virtual void save_model( FILE *fo ) {
            model.save_to_file( fo );
        }
        // initialize model by defined setting
        virtual void init_model( void ){
            model.check_init();  
        }        
        // initialize trainer before training 
        virtual void init_trainer( void ){
            feat_s.resize( model.param.num_global + model.param.num_spec_sparse );
            fcommon_s.resize( model.param.num_ufeedback );
            feat.set_state( &feat_s[0], model.param.num_global + model.param.num_spec_sparse );
            fcommon.set_state( &fcommon_s[0], model.param.num_ufeedback );
            
            dmat.set_column_len( model.param.num_global );
            for( int i = 0; i < fcommon.size(); i ++ ){
                fcommon.set_unknown( i );
            }
            for( int i = 0; i < feat.size(); i ++ ){
                feat.set_unknown( i );
            }
            if( strcmp( tax_name, "NULL" ) ){
                if( model.param.use_tax_root != 0 ) tax.load( tax_name );
            }else{
                apex_utils::assert_true( model.param.use_tax_root == 0, "no taxonomy information available for tax root" );
            }
        }
    private:
        // make dense feature from global feature 
        inline void build_dense( apex_rt::FVector feat, const SVDFeatureCSR::Elem &e, int fbase = 0, unsigned gstart = 0, unsigned gend = UINT_MAX ){
            int top = fbase;
            int base = model.param.baseline_mode == 0 ? 0 : 1;

            // weight type
            if( model.param.num_root_weight != 0 ){
                base = model.param.num_root_weight + 1;
                apex_utils::assert_true( e.num_global >= base && e.index_global[ base-1 ] == (unsigned)(base-1), 
                                         "not sufficient weight provided in global feature" ); 
            }
            
            for( int i = base; i < e.num_global; i ++ ){
                const unsigned gid = e.index_global[i] - base + fbase;
                // bound check
                if( gid < gstart || gid >= gend ) continue;
                apex_utils::assert_true( gid < (unsigned)model.param.num_global, "global index exceed bound" );
                while( top < (int)gid ){
                    feat.set_unknown( top ++ );
                }
                feat[ top ++ ] = e.value_global[i];
            }
            while( top < model.param.num_global ){
                feat.set_unknown( top ++ );
            }
        }
        
        // generate prediction
        inline float forward( const SVDFeatureCSR::Elem &e, bool use_res_buf_train ){
            unsigned gid = 0;
            if( model.param.num_item != 0 ){
                apex_utils::assert_true( e.num_ifactor == 1, "need exact 1 item id to specify item" );
                gid = e.index_ifactor[0];                
            }
            double sum = model.param.baseline_mode == 1 ? e.value_global[0] * scale_baseline : base_score;
            // add sparse part
            for( int i = 0; i < e.num_ufactor; i ++ ){
                apex_utils::assert_true( e.index_ufactor[i] < (unsigned)model.param.num_spec_sparse, "spec_sparse index exceed bound" );
                feat[ e.index_ufactor[i] ] = e.value_ufactor[ i ];
            }
            // add dense part 
            this->build_dense( feat, e, model.param.num_spec_sparse );            

            if( this->pred_tree_leaf == -1 ){
                // res buf train
                size_t istart = 0;               
                if( use_res_buf_train ){
                    res_buf_train.next();
                    if( !res_buf_train.is_first_round() ){
                        istart = model.trees.size() - 1;
                        sum = res_buf_train.curr();
                    }
                }
                
                for( size_t i = istart; i < model.trees.size(); i ++ ){
                    // weight type 
                    float weight = 1.0f;
                    if( model.param.num_root_weight != 0 && model.weight_type[i] != 0 ){
                        weight = e.value_global[ model.weight_type[i] ];
                    }
                    
                    if( model.param.use_tax_root == 0 ){                    
                        if( model.param.item_feature_mode < 3 ){
                            sum += model.trees[i]->predict( feat, fcommon, gid ) * weight;
                        }
                    }else{
                        sum += model.trees[i]->predict( feat, fcommon, tax[ gid ][ model.root_type[i] ] ) * weight;
                    }
                }

                // res buf train
                if( use_res_buf_train ){
                    res_buf_train.curr() = sum;
                }
            }else{
                apex_utils::assert_true( pred_tree_leaf < (int)model.trees.size(), "tree id exceed bound" );
                sum = model.trees[ pred_tree_leaf ]->get_leaf_id( feat, fcommon, gid );
            }
            // remove sparse part
            for( int i = 0; i < e.num_ufactor; i ++ ){
                feat.set_unknown( e.index_ufactor[i] );
            }

            return static_cast<float>( sum );
        }
        // create a new reg tree
        inline apex_rt::IRTTrainer *new_rt( void ){
            apex_rt::IRTTrainer *rt = apex_rt::create_rt_trainer( model.param.tree_type );
            cfg.before_first();
            while( cfg.next() ){
                rt->set_param( cfg.name(), cfg.val() );
            }
            char s[ 256 ];

            sprintf( s, "%f", param.learning_rate );
            rt->set_param( "learning_rate", s );

            sprintf( s, "%d", model.param.num_ufeedback );
            rt->set_param( "rt_num_group_sparse", s );
            sprintf( s, "%d", model.param.num_spec_sparse );
            rt->set_param( "rt_num_spec_sparse", s );
            
            if( model.param.use_tax_root == 0 ){
                sprintf( s, "%d", model.param.num_item == 0 ? 1 : model.param.num_item );
                rt->set_param( "rt_num_group", s );
            }else{
                sprintf( s, "%d", tax.size( rscheduler.curr_type() ) );
                rt->set_param( "rt_num_group", s );
            }
            rt->init_trainer();
            return rt;
        }
    protected:
        // add instance to sparse matrix 
        inline void add_instance( const SVDFeatureCSR::Elem &e, int gid, float grad, float sgrad, float weight ){
            // add it to feature matrix
            size_t idx = dmat.add_row( spart_index ); 

            const GBRTParamScheduler::Entry &pe = pscheduler.curr_type();
            this->build_dense( dmat[idx], e, 0, pe.gstart, pe.gend );
            
            // add extra sparse part if any
            if( model.param.num_spec_sparse != 0 ){
                apex_rt::FVectorSparse sp;
                sp.findex = e.index_ufactor;
                sp.fvalue = e.value_ufactor;
                sp.len    = e.num_ufactor;
                apex_utils::assert_true( this->dsmat.add_row( sp ) == idx, "BUG" );
            }
            
            if( gid >= 0 ){
                if( model.param.use_tax_root == 0 ){                            
                    dgroup_id.push_back( gid );
                }else{
                    dgroup_id.push_back( tax[ gid ][ rscheduler.curr_type() ] );
                }                        
            }
            // add negative to the weight
            dgrad.push_back( - grad );
            dgrad_second.push_back( - sgrad );
            
            // we don't need weight when loss type == 1
            if( rt_loss_type == 0 ){
                dweight.push_back( weight ); 
            }            
        }
        // add relavant gradient to the training batch
        inline void add_batch( const SVDPlusBlock &data ){
            for( int i = 0; i < data.data.num_row; i ++ ){
                SVDFeatureCSR::Elem e = data.data[i];
                if( tmp_weight[i] > 1e-5f ) {
                    float grad  = tmp_grad[i];
                    float sgrad = tmp_sgrad[i]; 
                    float weight = tmp_weight[i];

                    // weight type
                    if( model.param.num_root_weight != 0 ){
                        const int wt = wscheduler.curr_type();
                        // add weight to batch
                        if( wt != 0 ){
                            const float v = e.value_global[ wt ];
                            grad *= v; sgrad *= v*v; weight *= v*v;
                        }
                    }
                    
                    if( model.param.num_item != 0 ){
                        apex_utils::assert_true( e.num_ifactor == 1, "need exact 1 item id to specify item" );
                        const unsigned gid = e.index_ifactor[0];
                        if( model.param.item_feature_mode < 3 ){
                            this->add_instance( e, gid, grad, sgrad, weight );
                        }
                    }else{
                        this->add_instance( e, -1, grad, sgrad, weight );
                    }
                }
            }
        }
    protected:
        /*!
         * \brief update gradients and second gradients and weight into tmp_grad, tmp_sgrad, tmp_weight
         * \param tmp_pred prediction for each instance in data
         * \param data data block
         */
        virtual void update_stats( const std::vector<float> &tmp_pred, const SVDPlusBlock &data ) = 0;
    private:
        virtual void forward( std::vector<float> &p, const SVDPlusBlock &data, bool use_res_buf_train = false ){ 
            // add trace to fcommon
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                for( int i = 0; i < data.num_ufeedback; i ++ ){
                    apex_utils::assert_true( data.index_ufeedback[i] < (unsigned)model.param.num_ufeedback, "ufeeback index exceed bound" );
                    fcommon[ data.index_ufeedback[i] ] = data.value_ufeedback[ i ];
                }
            }
            
            p.resize( data.data.num_row );
            
            for( int i = 0; i < data.data.num_row; i ++ ){
                p[ i ] = this->forward( data.data[i], use_res_buf_train );
            }
            // remove trace from fcommon
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::END_TAG ){
                for( int i = 0; i < data.num_ufeedback; i ++ ){
                    fcommon.set_unknown( data.index_ufeedback[i] );
                }
            }   
        }        
    public:
        virtual void update( const SVDPlusBlock &data ){
            // add sparse part at beginning of ufeeback information
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                apex_rt::FVectorSparse sp;
                sp.findex = data.index_ufeedback;
                sp.fvalue = data.value_ufeedback;
                sp.len    = data.num_ufeedback;
                const GBRTParamScheduler::Entry &e = pscheduler.curr_type(); 
                this->spart_index = dmat.add_spart( sp, e.fstart, e.fend );
            }

            this->forward( tmp_pred, data, use_res_buf != 0 );
            {// tmp space setting
                tmp_grad.resize( data.data.num_row );
                tmp_weight.resize( data.data.num_row );
                tmp_sgrad.resize( data.data.num_row );
                std::fill( tmp_grad.begin(), tmp_grad.end(), 0.0f );
                std::fill( tmp_sgrad.begin(), tmp_sgrad.end(), 0.0f );
                std::fill( tmp_weight.begin(), tmp_weight.end(), 0.0f );
            }
            this->update_stats( tmp_pred, data );            
            this->add_batch( data );
        }
        virtual void predict( std::vector<float> &p, const SVDPlusBlock &data ){ 
            this->forward( p, data );
            for( int i = 0; i < data.data.num_row; i ++ ){
                p[ i ] = active_type::map_active( p[ i ] , model.mtype.active_type ); 
            }
        }
        virtual void set_round( int nround ){
            dgrad.resize( 0 ); 
            dgrad_second.resize( 0 );
            dmat.clear(); dsmat.clear();
            dgroup_id.resize( 0 );
            dweight.resize( 0 );
            rscheduler.set_round( nround );
            pscheduler.set_round( nround );
            wscheduler.set_round( nround );
            param.set_round( nround );
        }
        virtual void finish_round( void ){
            // try to train a new tree
            apex_rt::IRTTrainer *rt = this->new_rt();
            // train the rt
            rt->do_boost( dgrad, dgrad_second, dmat, dgroup_id, dweight, dsmat );
            
            int wtype = model.param.num_root_weight != 0 ? wscheduler.curr_type(): -1;
            if( model.param.use_tax_root == 0 ){
                model.push_back( rt, -1, wtype );
            }else{
                model.push_back( rt, rscheduler.curr_type(), wtype );
            }
            // res buf train
            res_buf_train.finish_round();
        }
    };
};

namespace apex_svd{
    // trainer that do regression or classification training
    class RegGBRTTrainer: public GBRTTrainer{
    private:
        float keep_prob;
    protected:
        virtual void update_stats( const std::vector<float> &tmp_pred, const SVDPlusBlock &data ){
            if( keep_prob < 1.0f - 1e-6f ){
                if( apex_random::sample_binary( keep_prob ) == 0 ) return;
            }
            for( int i = 0; i < data.data.num_row; i ++ ){
                float label = data.data[i].label;
                float pred   = active_type::map_active( tmp_pred[ i ], model.mtype.active_type );
                float err    = active_type::cal_grad ( label, pred, model.mtype.active_type );
                float sgrad  = active_type::cal_sgrad( label, pred, model.mtype.active_type );                
                tmp_grad[ i ]   = err;
                tmp_sgrad[ i ]  = sgrad;
                tmp_weight[ i ] = 1.0f;
            }
        }
    public:
        RegGBRTTrainer( const SVDTypeParam &mtype )
            :GBRTTrainer( mtype ){
            this->keep_prob = 1.0f;
        }
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "subsample_prob" ) )    keep_prob = (float)atof( val );
            GBRTTrainer::set_param( name, val );
        }
    }; 
};

namespace apex_svd{
    class LambdaGBRTTrainer: public GBRTTrainer{ 
    private:
        // whether use pointwise sample mode
        int sample_pointwise;
        // how to make use of lambda rank weight
        int lambda_weight_mode;
    protected:
        /*! \brief entry for information used to generate lambda rank sample */
        struct Entry{
            /*! \brief predicted score */
            float score;
            /*! \brief label of current entry */            
            float label;
            /*! \brief index of current entry in the dataset */
            unsigned data_index;
            /*! \brief compare score by default */            
            inline bool operator<( const Entry &p )const{
                return score > p.score;
            }
        };
        /*! \brief sample for lambda rank */
        struct LambdaSample{
            /*! \brief weight for this sample, by default set it to 1.0 */
            float weight;
            /*! \brief data index of positive sample */
            unsigned pos_index;
            /*! \brief data index of negative sample */
            unsigned neg_index;
            /*! \brief constructor */
            LambdaSample(){}
            LambdaSample( unsigned pos_index, unsigned neg_index, float weight = 1.0f ){
                this->pos_index = pos_index;
                this->neg_index = neg_index;
                this->weight = weight;
            }
        };
    private:
        // update step, one pair
        inline void update_grad( const SVDPlusBlock &data, const LambdaSample &sample ){
            // skip samples with too small weight 
            if( sample.weight < 1e-5f ) return;
            // pairwise mode
            if( sample_pointwise == 0 ) {
                float pred   = active_type::map_active( tmp_pred[ sample.pos_index ] - tmp_pred[ sample.neg_index ], model.mtype.active_type );
                float err    = active_type::cal_grad ( 1.0f, pred, model.mtype.active_type ) * sample.weight;
                float sgrad  = active_type::cal_sgrad( 1.0f, pred, model.mtype.active_type ) * sample.weight;
                
                tmp_grad[ sample.pos_index ] += err;
                tmp_grad[ sample.neg_index ] -= err;
                tmp_sgrad[ sample.pos_index ]+= sgrad;
                tmp_sgrad[ sample.neg_index ]+= sgrad;
            }else{
                {
                    float pp = active_type::map_active( tmp_pred[ sample.pos_index ], model.mtype.active_type );
                    tmp_grad[ sample.pos_index ] += active_type::cal_grad( 1.0f, pp, model.mtype.active_type ) * sample.weight;
                    tmp_sgrad[ sample.pos_index ] += active_type::cal_sgrad( 1.0f, pp, model.mtype.active_type ) * sample.weight;
                }
                {
                    float np = active_type::map_active( tmp_pred[ sample.neg_index ], model.mtype.active_type );
                    tmp_grad[ sample.neg_index ] += active_type::cal_grad( 0.0f, np, model.mtype.active_type ) * sample.weight;
                    tmp_sgrad[ sample.neg_index ] += active_type::cal_sgrad( 0.0f, np, model.mtype.active_type ) * sample.weight;
                }                
            }
            // which weight mode to use
            if( lambda_weight_mode == 0 ){
                tmp_weight[ sample.pos_index ] += 1.0f;
                tmp_weight[ sample.neg_index ] += 1.0f;
            }else{
                tmp_weight[ sample.pos_index ] += sample.weight;
                tmp_weight[ sample.neg_index ] += sample.weight;
            }
        }                
    protected:
        /*!
         * \brief generate samples given the information in the data
         * \param samples output samples for lambda rank
         * \param data input information for sample generation, sorted by score
         */
        virtual void gen_sample( std::vector<LambdaSample> &samples, std::vector<Entry> &data, bool is_attach ) = 0;
    protected:
        virtual void update_stats( const std::vector<float> &tmp_pred, const SVDPlusBlock &data ){
            std::vector<LambdaSample> samples;
            {// get prediction for each data
                Entry e;
                std::vector<Entry> info;
                for( int i = 0; i < data.data.num_row; i ++ ){
                    e.score = tmp_pred[i];
                    e.label = data.data[i].label;
                    e.data_index = static_cast<unsigned>( i );
                    info.push_back( e );
                }
                std::sort( info.begin(), info.end() );
                this->gen_sample( samples, info, data.extra_info != 0 );
            }
            
            {// update samples                
                for( size_t i = 0; i < samples.size(); i ++ ){
                    this->update_grad( data, samples[i] );
                }
            }            
        }
    public:
        LambdaGBRTTrainer( const SVDTypeParam &mtype )
            :GBRTTrainer( mtype ){
            this->lambda_weight_mode = 1;
            this->sample_pointwise = 0;
        }
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "lambda_weight_mode") ) lambda_weight_mode = atoi( val ); 
            if( !strcmp( name, "rank_sample_pointwise") ) sample_pointwise = atoi( val );             
            GBRTTrainer::set_param( name, val );
        }     
    };
};

namespace apex_svd{
    class APLambdaGBRTTrainer : public LambdaGBRTTrainer{
    private:
        // sample strategies
		int ap_method;
		int ap_maxn;
        int sample_num;
        int attach_sample_num;
        int reject_method;
		int ap_start_round;
		int nround;
		float ap_alpha;
        float keep_prob;
    public:
        APLambdaGBRTTrainer( const SVDTypeParam &mtype ):
            LambdaGBRTTrainer( mtype ){
            sample_num = -1;
			ap_alpha = 0.0f;
            ap_maxn  = INT_MAX;
            ap_method = 0;
            reject_method = 0;
			ap_start_round = 0;
            keep_prob = 1.0f;
        }
        virtual ~APLambdaGBRTTrainer(){}
	private:
		inline void gen_sweight( std::vector<LambdaSample> &samples, std::vector<Entry> &data, bool is_attach ){
            // random drop off samples for bagging
            if( keep_prob < 1.0f - 1e-6f ){
                if( apex_random::sample_binary( keep_prob ) == 0 ) return;
            }
            // simple implementation using uniform sampling
            std::vector<int> pos, neg, pos_top;
            for( size_t i = 0; i < data.size(); i ++ ){
                if( data[i].label > 0.5f ){
                    pos.push_back( (int)i );
                    if( (int)i < ap_maxn ){  
                        pos_top.push_back( (int)i );
                    }
                }else{
                    neg.push_back( (int)i );
                }
            }            
            // start generate pairs
            if( pos.size() > 0 && neg.size() > 0 ){
                apex_random::shuffle( pos );
				apex_random::shuffle( neg );

                size_t snum = 0;
                if( sample_num > 0 ) snum = is_attach ? (size_t) attach_sample_num: (size_t) sample_num;
                if( sample_num == -1 ) snum = neg.size();
                if( sample_num == -2 ) snum = pos.size();
                
                size_t nsample = 0;
                for( size_t i = 0; nsample < snum; i ++ ){
					float delta_ap = 0.0, wt;
					if( nround >= ap_start_round ){
						int pos_idx = pos[ i % pos.size() ];
						int neg_idx = neg[ i % neg.size() ];

						if( pos_idx < neg_idx ) std::swap( pos_idx, neg_idx );                        
                        if( neg_idx < ap_maxn ){
                            // try to calculate delta ap
                            int pos_cnt = 0;
                            for( size_t j = 0; j < pos_top.size(); j ++ ){
                                if( pos_top[j] >= pos_idx ){
                                    delta_ap -= ( j + 1.0f ) / ( pos_idx + 1.0f ); break;
                                }
                                if( pos_top[j] > neg_idx ){
                                    delta_ap += 1.0f /( pos_top[j] + 1.0f ); 
                                }else{
                                    if( pos_top[j] != neg_idx ) pos_cnt ++;
                                }				
                            }
                            delta_ap += ( pos_cnt + 1.0f ) / ( neg_idx + 1.0f );
                            delta_ap /= pos.size();
                        }
                        apex_utils::assert_true( delta_ap < 1.0f + 1e-6f, "BUGA" ); 
                        apex_utils::assert_true( delta_ap ==  delta_ap, "BUGB" ); 
                        wt = ap_alpha * delta_ap + 1.0f - ap_alpha;
                    }else{
                        wt = 1.0f;                        
                    }
                    int pos_idx = pos[ i % pos.size() ];
                    int neg_idx = neg[ i % neg.size() ];
					// use old way in the begining						
                    switch( reject_method ){
                    case 0: samples.push_back( LambdaSample( data[ pos_idx ].data_index, data[ neg_idx ].data_index, wt ) ); nsample ++; break;
                    case 1: {
                        if( apex_random::sample_binary( wt ) != 0 ){
                            samples.push_back( LambdaSample( data[ pos_idx ].data_index, data[ neg_idx ].data_index ) ); 
                        }
                        nsample ++; break;
                    }
                    case 2: {
                        if( apex_random::sample_binary( wt ) != 0 ){
                            samples.push_back( LambdaSample( data[ pos_idx ].data_index, data[ neg_idx ].data_index ) ); nsample ++;
                        }
                        break;
                    }
                    default: apex_utils::error("reject method unknown");
                    }
                }
            }
		}
    protected:
        // implement me: this is an example implementation
        virtual void gen_sample( std::vector<LambdaSample> &samples, std::vector<Entry> &data, bool is_attach ){
			switch( ap_method ){
			case 0: gen_sweight( samples, data, is_attach ); break;
			default: apex_utils::error( "unknown ap method\n");
			}
        }        
    public:
        // set param here
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name, "rank_sample_num" ) ) sample_num = atoi( val );
            if( !strcmp( name, "attach:rank_sample_num" ) ) attach_sample_num = atoi( val );
            if( !strcmp( name, "lambda_ap_maxn" ) )    ap_maxn = atoi( val );
            if( !strcmp( name, "lambda_ap_method" ) )  ap_method = atoi( val );
            if( !strcmp( name, "lambda_ap_alpha"  ) )  ap_alpha  = (float)atof( val );
            if( !strcmp( name, "lambda_ap_reject" ) )  reject_method = atoi( val );
            if( !strcmp( name, "lambda_ap_rstart" ) )  ap_start_round = atoi( val );
            if( !strcmp( name, "lambda_keep_prob" ) )  keep_prob = (float)atof( val );
            if( !strcmp( name, "subsample_prob" ) )    keep_prob = (float)atof( val );
            LambdaGBRTTrainer::set_param( name, val );
        }
		virtual void set_round( int nround ){
			LambdaGBRTTrainer::set_round( nround );
			this->nround = nround;
		}
    };
};

#endif

