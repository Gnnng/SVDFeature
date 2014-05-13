/*
 *  Copyright 2009-2010 APEX Data & Knowledge Management Lab, Shanghai Jiao Tong University
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
 * \author Tianqi Chen: tqchen@apex.sjtu.edu.cn
 */
#ifndef _APEX_SVD_BASE_H_
#define _APEX_SVD_BASE_H_

#include "../../apex_svd.h"
#include <cstring>

namespace apex_svd{
    using namespace apex_tensor;
    using namespace apex_utils;
};

namespace apex_svd{
    // parameter set, used for detailed tuning
    class ParameterSet{
    private:
        // weight decay
        std::vector<float>    wd;
        // bound for index 
        std::vector<unsigned> bound;
        int  prefix_lenA, prefix_lenB;
        char prefixA[ 256 ], prefixB[ 256 ];
    public :
        ParameterSet( const char *prefixA, const char *prefixB ){
            strcpy( this->prefixA, prefixA );
            strcpy( this->prefixB, prefixB );
            this->prefix_lenA = static_cast<int>( strlen( prefixA ) );
            this->prefix_lenB = static_cast<int>( strlen( prefixB ) );
        }
        inline void set_param( const char *name, const char *val ){
            if( !strncmp( name, prefixA, prefix_lenA ) ){
                name += prefix_lenA;
            }else{
                if( !strncmp( name, prefixB, prefix_lenB ) ){
                    name += prefix_lenB;
                }else return;
            }

            if( !strcmp( "bound", name ) ){
                unsigned bd = (unsigned)atoi(val);
                apex_utils::assert_true( bd > 0, "can't give 0 as bound" ); 
                apex_utils::assert_true( bound.size()==0||bound.back() < bd , "bound must be given in order" );
                apex_utils::assert_true( bound.size()+1 == wd.size() , "must specifiy wd in each range" );
                bound.push_back( bd - 1 );
            }
            if( !strcmp( "wd", name ) ){
                apex_utils::assert_true( wd.size() == bound.size(), "setting must be exactly" );
                wd.push_back( (float)atof(val) );
            }
        }
        inline float get_wd( unsigned gid, float wd_default ) const{
            if( bound.size() == 0 ) return wd_default;
            size_t idx = std::lower_bound( bound.begin(), bound.end(), gid ) - bound.begin();
            apex_utils::assert_true( idx < bound.size() , "bound set err" );
            return wd[ idx ];
        }
    };
};
// this file defines the main algorithm of toolkit
namespace apex_svd{    
    class SVDFeature: public ISVDTrainer{
    protected:
        int init_end;
        SVDModel model;
        SVDTrainParam param;
    protected:
        CTensor1D tmp_ufactor, tmp_ifactor;
        // extend hierachical feature associated with each user/item
        SparseFeatureArray<float> feat_user, feat_item;
    private:
        int round_counter;
    private:
        char name_feat_user[ 256 ];
        char name_feat_item[ 256 ];
    protected:
        // data structure used for lazy decay
        unsigned sample_counter;
    private:
        unsigned *ref_user, *ref_item, *ref_global;
    private:
        // data structure for detail weight decay tuning
        ParameterSet u_param, i_param, g_param;
    public:
        SVDFeature( const SVDTypeParam &mtype ):
            u_param("up:","uip:"),i_param("ip:","uip:"),g_param("gp:","gp:"){
            model.mtype = mtype;
            strcpy( name_feat_user, "NULL" );
            strcpy( name_feat_item, "NULL" );
            this->round_counter = 0;
            this->init_end = 0;
        }
        virtual ~SVDFeature(){
            model.free_space();
            if( init_end == 0 ) return;
            tensor::free_space( tmp_ufactor );
            tensor::free_space( tmp_ifactor );
            // lazy decay
            if( param.reg_global >= 4 ) {
                delete [] ref_global;
            } 
            if( param.reg_method >= 4 ){
                delete [] ref_user; 
                if( model.param.common_latent_space == 0 ) delete [] ref_item;
            }
        }
    public:
        // model related interface
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name,"feature_user" )) strcpy( name_feat_user  , val ); 
            if( !strcmp( name,"feature_item" )) strcpy( name_feat_item  , val ); 
            param.set_param( name, val );
            u_param.set_param( name, val );
            i_param.set_param( name, val );
            g_param.set_param( name, val );
            if( model.space_allocated == 0 ){
                model.param.set_param( name, val );
            }
        }
        // load model from file
        virtual void load_model( FILE *fi ) {
            model.load_from_file( fi );
        }
        // save model to file
        virtual void save_model( FILE *fo ) {
            model.save_to_file( fo );
        }
        // initialize model by defined setting
        virtual void init_model( void ){
            model.alloc_space();
            model.rand_init();
        }
        // initialize trainer before training 
        virtual void init_trainer( void ){
            if( strcmp( name_feat_user , "NULL") ) feat_user.load( name_feat_user );
            if( strcmp( name_feat_item , "NULL") ) feat_item.load( name_feat_item );
            tmp_ufactor = clone( model.W_user[0] );
            tmp_ifactor = clone( model.W_item[0] );
            // lazy decay
            this->sample_counter = 0;
            if( param.reg_global >= 4 ) {
                ref_global = new unsigned[ model.param.num_global ];
                memset( ref_global, 0, sizeof(unsigned)*model.param.num_global );
            } 
            if( param.reg_method >= 4 ){
                ref_user   = new unsigned[ model.param.num_user ];
                if( model.param.common_latent_space == 0 ){
                    ref_item = new unsigned[ model.param.num_item ];
                }else{
                    ref_item = ref_user; 
                }
                memset( ref_user, 0, sizeof(unsigned)*model.param.num_user );
                memset( ref_item, 0, sizeof(unsigned)*model.param.num_item );
            }
            this->init_end = 1;
        }
    protected:        
        inline static void reg_L1( float &w, float wd ){
            if( w > wd ) w -= wd;
            else 
                if( w < -wd ) w += wd;
                else w = 0.0f;
        }
        inline static void project( CTensor1D w, float B ){
            float sum = cpu_only::dot( w, w );
            if( sum > B ){
                w *= sqrtf( B / sum );
            }            
        }
    private:
        inline void reg_global( const unsigned gid ){
            float lambda = param.learning_rate * g_param.get_wd( gid, param.wd_global );
            if( gid >= param.num_regfree_global ){ 
                switch( param.reg_global ){
                case 0: model.g_bias[ gid ] *= ( 1.0f - lambda ); break;
                case 1: reg_L1( model.g_bias[ gid ], lambda ); break;
                case 4: {// lazy L2 decay
                    float k = static_cast<float>( ref_global[gid] - sample_counter );
                    model.g_bias[ gid ] *= expf( logf( 1.0f - lambda ) * k );
                    ref_global[ gid ] = sample_counter;
                    break;
                }
                case 5: {// lazy L1 decay
                    float k = static_cast<float>( ref_global[gid] - sample_counter );
                    reg_L1( model.g_bias[ gid ], lambda * k );
                    ref_global[ gid ] = sample_counter;
                    break;                    
                }
                default:
                    apex_utils::error("unknown global decay method");
                }
            }
        }
        inline void reg_user( const unsigned uid ){
            // regularize factor 
            float wd =u_param.get_wd( uid, param.wd_user );
            float lambda = param.learning_rate * wd;
            switch( param.reg_method ){
            case 0: model.W_user[ uid ] *= ( 1.0f - lambda ); break;
			case 3: 
            case 1:{
                CTensor1D w;
                w = model.W_user[ uid ];
                tensor::regularize_L1( w, lambda ); 
                break;                
            }
            case 2: project( model.W_user[ uid ], wd ); break;
            case 4: {// lazy L2 decay
                float k = static_cast<float>( ref_user[uid] - sample_counter );
                model.W_user[ uid ] *= expf( logf( 1.0f - lambda ) * k );
                ref_user[ uid ] = sample_counter;
                break;
            }
            case 5: {// lazy L1 decay
                CTensor1D w;
                w = model.W_user[ uid ];
                float k = static_cast<float>( ref_user[uid] - sample_counter );
                tensor::regularize_L1( w, lambda * k );
                ref_user[ uid ] = sample_counter;
                break;                    
            }
            default:apex_utils::error( "unknown reg_method" );
            }

            if( model.param.user_nonnegative ) {
                CTensor1D w = model.W_user[ uid ];
                tensor::smaller_then_fill( w, 0.0f );
            }  
            // only do L2 decay for bias
            if( model.param.no_user_bias == 0 ){
                model.u_bias[ uid ] *= ( 1.0f - param.learning_rate * param.wd_user_bias );
            }
        }
        inline void reg_item( const unsigned iid ){
            CTensor1D w;           
            float wd = i_param.get_wd( iid, param.wd_item );
            float lambda = param.learning_rate * wd;
            switch( param.reg_method ){
			case 3:
            case 0: model.W_item[ iid ] *= ( 1.0f - lambda ); break;
            case 1:{
                CTensor1D w;
                w = model.W_item[ iid ];
                tensor::regularize_L1( w, lambda ); 
                break;
            }
            case 2: project( model.W_item[ iid ], wd ); break;
            case 4: {// lazy L2 decay
                float k = static_cast<float>( ref_item[iid] - sample_counter );
                model.W_item[ iid ] *= expf( logf( 1.0f - lambda ) * k );
                ref_item[ iid ] = sample_counter;
                break;
            }
            case 5: {// lazy L1 decay
                CTensor1D w;
                w = model.W_item[ iid ];
                float k = static_cast<float>( ref_item[iid] - sample_counter );
                tensor::regularize_L1( w, lambda * k );
                ref_item[ iid ] = sample_counter;
                break;                    
            }
            default:apex_utils::error( "unknown reg_method" );
            }
            // only do L2 decay for bias
            model.i_bias[ iid ] *= ( 1.0f - param.learning_rate * param.wd_item_bias );
        }

        // do regularization
        inline void regularize( const SVDFeatureCSR::Elem feature, bool is_after_update ){            
            // when reg_method>=3, regularization is performed before update
            if( ( is_after_update && param.reg_global < 4) || 
                (!is_after_update && param.reg_global >=4)  ){ 
                for( int i = 0; i < feature.num_global; i ++ ){
                    this->reg_global( feature.index_global[i] );
                }
            }
            if( ( is_after_update && param.reg_method < 4) || 
                (!is_after_update && param.reg_method >=4)  ){ 
                for( int i = 0; i < feature.num_ufactor; i ++ ){
                    this->reg_user( feature.index_ufactor[i] );
                    SparseFeatureArray<float>::Vector vec = feat_user[ feature.index_ufactor[i] ];
                    for( int j = 0; j < vec.size(); j ++ ){
                        this->reg_user( vec[j].index );
                    }
                }
                for( int i = 0; i < feature.num_ifactor; i ++ ){                
                    this->reg_item( feature.index_ifactor[i] );
                    SparseFeatureArray<float>::Vector vec = feat_item[ feature.index_ifactor[i] ];
                    for( int j = 0; j < vec.size(); j ++ ){
                        this->reg_item( vec[j].index );
                    }
                }
            }
        }
    private:
        inline double calc_bias( const SVDFeatureCSR::Elem &feature,
                                 const CTensor1D &u_bias, 
                                 const CTensor1D &i_bias, 
                                 const CTensor1D &g_bias ){
            double sum = 0.0f;
            for( int i = 0; i < feature.num_global; i ++ ){
                const unsigned gid = feature.index_global[i];
                apex_utils::assert_true( gid < (unsigned)model.param.num_global, "global feature index exceed setting" );
                sum += feature.value_global[i] * g_bias[ gid ];
            }
                        
            if( model.param.no_user_bias == 0 ){
                for( int i = 0; i < feature.num_ufactor; i ++ ){
                    const unsigned uid = feature.index_ufactor[i];
                    apex_utils::assert_true( uid < (unsigned)model.param.num_user, "user feature index exceed bound" );
                    sum += feature.value_ufactor[i] * u_bias[ uid ];
                    // extra feature
                    SparseFeatureArray<float>::Vector vec = feat_user[ uid ];
                    for( int j = 0; j < vec.size(); j ++ ){
                        sum += u_bias[ vec[j].index ] * vec[j].value;
                    }
                }
                sum += this->get_bias_svdpp();
            }
             
            sum += this->get_bias_plugin( feature );

            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                apex_utils::assert_true( iid < (unsigned)model.param.num_item, "item feature index exceed bound" );
                sum +=  ival * i_bias[ iid ];
                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    sum += i_bias[ vec[j].index ] * vec[j].value * ival;
                }
            }
            
            return sum;
        }
        inline void prepare_tmp( const SVDFeatureCSR::Elem &feature ){ 
            this->prepare_svdpp( tmp_ufactor );
            tmp_ifactor = 0.0f;

            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const unsigned uid = feature.index_ufactor[i];
                apex_utils::assert_true( uid < (unsigned)model.param.num_user, "user feature index exceed bound" );

                tmp_ufactor += model.W_user[ uid ] * feature.value_ufactor[i];

                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_user[ uid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    tmp_ufactor += model.W_user[ vec[j].index ] * vec[j].value;
                }               
            }            
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                tmp_ifactor += model.W_item[ iid ] * ival;

                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    tmp_ifactor += model.W_item[ vec[j].index ] * vec[j].value * ival;
                }
            }            
        }
                
        inline void update_no_decay( float err, const SVDFeatureCSR::Elem &feature  ){ 
            for( int i = 0; i < feature.num_global; i ++ ){
                const unsigned gid = feature.index_global[i];                
                model.g_bias[ gid ] += param.learning_rate * err * feature.value_global[i];
            }
                
            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const unsigned uid = feature.index_ufactor[i];                
                float scale = param.learning_rate * err * feature.value_ufactor[i];
                    
                model.W_user[ uid ] += scale * tmp_ifactor;

                if( model.param.no_user_bias == 0 ){ 
                    model.u_bias[ uid ] += scale;
                }
                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_user[ uid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    float sc = param.learning_rate * err * vec[j].value;                    
                    model.W_user[ vec[j].index ] += sc * tmp_ifactor;
                    if( model.param.no_user_bias == 0 ){
                        model.u_bias[ vec[j].index ] += sc;
                    }
                }
            }

            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                float scale = param.learning_rate * err * ival;
                model.W_item[ iid ] += scale * tmp_ufactor;                 
                model.i_bias[ iid ] += scale;

                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    float sc = param.learning_rate * err * vec[j].value * ival;
                    model.i_bias[ vec[j].index ] += sc;
                    model.W_item[ vec[j].index ] += sc * tmp_ufactor;
                }
            }
                        
            this->update_svdpp( err, tmp_ifactor );
            this->update_bias_plugin( err, feature );
        }
    protected:
        // implicit feedback information
        virtual void prepare_svdpp( CTensor1D &tmp_ufactor ){
            tmp_ufactor = 0.0f;
        }
        virtual float get_bias_svdpp( void ){
            return 0.0f;
        }
        virtual float get_bias_plugin( const SVDFeatureCSR::Elem &feature ){
            return 0.0f;
        }
        virtual void update_bias_plugin( float err, const SVDFeatureCSR::Elem &feature ){
        }
        virtual void update_svdpp( float err, const CTensor1D &tmp_ifactor ){
            // do nothing
        }
    protected:
        inline float pred( const SVDFeatureCSR::Elem &feature ){ 
            double sum = model.param.base_score + 
                this->calc_bias( feature, model.u_bias, model.i_bias, model.g_bias );
            
            this->prepare_tmp( feature );

            sum += apex_tensor::cpu_only::dot( tmp_ufactor, tmp_ifactor );            
            
            return active_type::map_active( (float)sum, model.mtype.active_type ); 
        }

        inline void update_inner( const SVDFeatureCSR::Elem &feature, float sample_weight = 1.0f ){ 
            this->regularize( feature, false );
            float err = active_type::cal_grad( feature.label, this->pred( feature ), model.mtype.active_type ) * sample_weight;
            this->update_no_decay( err, feature  );
            this->sample_counter ++;
            this->regularize( feature, true );
        }
    public:        
        virtual void update( const SVDFeatureCSR::Elem &feature ){             
            this->update_inner( feature );
        }
        virtual float predict( const SVDFeatureCSR::Elem &feature ){ 
            return this->pred( feature );
        }
        virtual void set_round( int nround ){
            if( param.decay_learning_rate != 0 ){
                apex_utils::assert_true( round_counter <= nround, "round counter restriction" );
                while( round_counter < nround ){
                    param.learning_rate *= param.decay_rate;
                    round_counter ++;
                }
            }
        }
    };
};

namespace apex_svd{
    // SVD++ plugin for SVDFeature
    class SVDPPFeature: public SVDFeature{
    private:
        float norm_ufeedback;
        float tmp_ufeedback_bias, old_ufeedback_bias;
        CTensor1D tmp_ufeedback, old_ufeedback;
    public:
        SVDPPFeature( const SVDTypeParam &mtype ):
            SVDFeature( mtype ){
        }
        virtual ~SVDPPFeature(){
            if( init_end == 0 ) return;            
            tensor::free_space( tmp_ufeedback );
            tensor::free_space( old_ufeedback );
        }
        // initialize trainer before training 
        virtual void init_trainer( void ){
            tmp_ufeedback = clone( model.W_user[0] );
            old_ufeedback = clone( model.W_user[0] );
            SVDFeature::init_trainer();            
        }        
    protected:
        // implicit feedback information
        virtual void prepare_svdpp( CTensor1D &tmp_ufactor ){
            tensor::copy( tmp_ufactor, tmp_ufeedback ); 
        }
        virtual float get_bias_svdpp( void ){
            return tmp_ufeedback_bias;
        }               
        virtual void update_svdpp( float err, const CTensor1D &tmp_ifactor ){
            float lr = param.learning_rate * param.scale_lr_ufeedback;
            tmp_ufeedback += lr * err * norm_ufeedback * tmp_ifactor;
            tmp_ufeedback *= ( 1.0f - lr * param.wd_ufeedback );
            if( model.param.no_user_bias == 0 ){
                tmp_ufeedback_bias += lr * err * norm_ufeedback;
                tmp_ufeedback_bias *= ( 1.0f  - lr * param.wd_ufeedback_bias );
            }            
        }
    protected:
        // SVD++ style
        inline void prepare_ufeedback( const SVDPlusBlock &data, unsigned start_fid = 0 ){ 
            norm_ufeedback = 0.0f;
            tmp_ufeedback = 0.0f; tmp_ufeedback_bias = 0.0f;
            for( int i = 0; i < data.num_ufeedback; i ++ ){
                const unsigned fid = data.index_ufeedback[i];
                if( fid < start_fid ) continue;
                const float    val = data.value_ufeedback[i];
                apex_utils::assert_true( fid < static_cast<unsigned>( model.param.num_ufeedback ), 
                                         "ufeedback id exceed bound" );
                tmp_ufeedback += model.W_ufeedback[ fid ] * val;
                norm_ufeedback+= val * val;
                if( model.param.no_user_bias == 0 ){
                    tmp_ufeedback_bias += model.ufeedback_bias[ fid ] * val;
                }
            }            
        }        
        inline void update_ufeedback( const SVDPlusBlock &data, unsigned start_fid = 0 ){ 
            if( data.num_ufeedback == 0 ) return;
            tmp_ufeedback -= old_ufeedback;
            tmp_ufeedback_bias -= old_ufeedback_bias;
            tmp_ufeedback *= 1.0f/ norm_ufeedback;
            tmp_ufeedback_bias *= 1.0f / norm_ufeedback;
            for( int i = 0; i < data.num_ufeedback; i ++ ){
                const unsigned fid = data.index_ufeedback[i];
                if( fid < start_fid ) continue;
                const float    val = data.value_ufeedback[i];
                model.W_ufeedback[ fid ] += tmp_ufeedback * val;
                if( model.param.no_user_bias == 0 ){
                    model.ufeedback_bias[ fid ] += tmp_ufeedback_bias * val;
                }
            }
        }
    protected:
        /*!
         * \brief update each instance in the block, 
         *        can be overwrite
         */
        virtual void update_each( const SVDPlusBlock &data ){ 
            // update 
            for( int i = 0; i < data.data.num_row; i ++ ){
                this->update_inner( data.data[i] );
            }            
        }
    public:
        // SVD++ style
        virtual void update( const SVDPlusBlock &data ){ 
            // calculate and backup feedback in the first block
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                this->prepare_ufeedback( data );
                old_ufeedback_bias = tmp_ufeedback_bias;
                tensor::copy( old_ufeedback, tmp_ufeedback );            
            }

            this->update_each( data );

            // update feedback data in the last block
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::END_TAG ){
                this->update_ufeedback( data );
            }
        }
        virtual void predict( std::vector<float> &p, const SVDPlusBlock &data ){ 
            p.clear();
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                this->prepare_ufeedback( data );
            }
            for( int i = 0; i < data.data.num_row; i ++ ){
                p.push_back( this->pred( data.data[i] ) );
            }
        }        
    };
};

namespace apex_svd{
    // implementation of ranker for SVDFeature
    class SVDFeatureRanker: public ISVDRanker{
    private:
        SVDModel model;
        // extend hierachical feature associated with each user/item
        SparseFeatureArray<float> feat_user, feat_item;
    private:
        char name_feat_user[ 256 ];
        char name_feat_item[ 256 ];
    private:
        // whether all the allocations has been done
        int init_end;
        // number of item set
        int num_item_set;
        // calculated factor part and bias part of the items
        CTensor2D tmp_ifactors;
        CTensor1D bias_ifactors;
        // user factor data here
        CTensor1D tmp_ufactor, tmp_ifactor, tmp_ufeedback;        
    private:
        struct Entry{
            int iid;
            float score;
            Entry( int iid, float score ){ 
                this->iid=iid; this->score = score; 
            } 
            inline bool operator<( const Entry &p )const{ return score > p.score; }  
        };
        // number of item processed
        int num_item_processed, top_k;
        // tag entry of the items
        int *item_tag;        
        // record the set of pos item set
        std::vector<int> pos_item;
        // store the scores of itemset
        CTensor1D item_score;
    public:
        SVDFeatureRanker( const SVDTypeParam &mtype ){
            model.mtype = mtype;
            strcpy( name_feat_user, "NULL" );
            strcpy( name_feat_item, "NULL" );
            this->init_end = 0;
            this->top_k    = 0;
        }
        virtual ~SVDFeatureRanker(){
            model.free_space();
            if( init_end == 0 ) return;
            tensor::free_space( tmp_ufactor );
            tensor::free_space( tmp_ifactor );
            tensor::free_space( tmp_ifactors );
            tensor::free_space( bias_ifactors );
            tensor::free_space( item_score );
            delete []item_tag;
            // SVD++ style
            if( model.mtype.format_type == svd_type::USER_GROUP_FORMAT ){
                tensor::free_space( tmp_ufeedback );
            }
        }
    public:
        // model related interface
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name,"feature_user" )) strcpy( name_feat_user  , val ); 
            if( !strcmp( name,"feature_item" )) strcpy( name_feat_item  , val ); 
            if( !strcmp( name,"top_k" )) top_k = atoi( val );
        }
        // load model from file
        virtual void load_model( FILE *fi ) {
            model.load_from_file( fi );
        }
        // initialize trainer before ranking
        virtual void init_ranker( int num_item_set ){
            if( strcmp( name_feat_user , "NULL") ) feat_user.load( name_feat_user );
            if( strcmp( name_feat_item , "NULL") ) feat_item.load( name_feat_item );
            // allocate necessary space for the item set
            this->num_item_processed = 0;
            this->num_item_set = num_item_set;
            tmp_ufactor = clone( model.W_user[0] );
            tmp_ifactor = clone( model.W_item[0] );
            tmp_ifactors.set_param( num_item_set, model.param.num_factor );
            bias_ifactors.set_param( num_item_set );
            tensor::alloc_space( tmp_ifactors  );
            tensor::alloc_space( bias_ifactors );
            item_score = clone( bias_ifactors );
            item_tag   = new int[ num_item_set ];
            // SVD++ style
            if( model.mtype.format_type == svd_type::USER_GROUP_FORMAT ){
                tmp_ufeedback = clone( model.W_user[0] );
            }
            this->init_end = 1;
        }
    private:
        inline void prepare_ifactor( CTensor1D ifactor, float &bias, const SVDFeatureCSR::Elem &feature ){
            ifactor = 0.0f;
            bias    = 0.0f;
            // prepare the data for item factors 
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                apex_utils::assert_true( iid < (unsigned)model.param.num_item, "item feature index exceed setting" );
                ifactor += model.W_item[ iid ] * ival;
                bias += model.i_bias[ iid ] * ival;
                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    ifactor += model.W_item[ vec[j].index ] * vec[j].value * ival;
                    bias += model.i_bias[ vec[j].index ] * vec[j].value * ival;
                }
            }
            // add influence of global factors
            for( int i = 0; i < feature.num_global; i ++ ){
                const unsigned gid = feature.index_global[i];
                apex_utils::assert_true( gid < (unsigned)model.param.num_global, "global feature index exceed setting" );
                bias += feature.value_global[i] * model.g_bias[ gid ];
            }
        }
        // process a new itemset
        inline void proc_item( const SVDFeatureCSR::Elem &feature ){
            // store the result to this index
            const int idx = this->num_item_processed ++;
            apex_utils::assert_true( num_item_processed <= num_item_set, "item instance exceed specified item set size" ); 
            this->prepare_ifactor( tmp_ifactors[idx], bias_ifactors[idx], feature );
        }
        // process user
        inline void proc_user( const SVDFeatureCSR::Elem &feature ){
            // SVD++ style
            if( model.mtype.format_type == svd_type::USER_GROUP_FORMAT ){
                tensor::copy( tmp_ufactor, tmp_ufeedback ); 
            }else{
                tmp_ufactor = 0.0f;
            }            
            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const unsigned uid = feature.index_ufactor[i];
                apex_utils::assert_true( uid < (unsigned)model.param.num_user, "user feature index exceed bound" );
                tmp_ufactor += model.W_user[ uid ] * feature.value_ufactor[i];
                // extra feature
                SparseFeatureArray<float>::Vector vec = feat_user[ uid ];
                for( int j = 0; j < vec.size(); j ++ ){
                    tmp_ufactor += model.W_user[ vec[j].index ] * vec[j].value;
                }               
            }            
            // initialize the auxiliary information
            pos_item.clear();
            item_score = 0.0f;
            std::fill( item_tag, item_tag + num_item_processed, 0 );            
        }        
        inline void proc_tag( const SVDFeatureCSR::Elem &feature, int tag ){
            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const int idx = feature.index_ufactor[i];
                apex_utils::assert_true( idx < this->num_item_processed, "sample item index exceed bound" );
                apex_utils::assert_true( item_tag[ idx ] == 0, "each pos sample item can not occur in baned sample list" );
                this->item_tag[ idx ] = tag;
                if( tag == svdranker_tag::POS_SAMPLE ) pos_item.push_back( idx );
            }
        }
        inline void proc_spec( const SVDFeatureCSR::Elem &feature ){
            apex_utils::assert_true( feature.num_ufactor == 1, 
                                     "must specify item index of sample in user feature field\n" );
            const int idx = feature.index_ufactor[0];
            apex_utils::assert_true( idx < this->num_item_processed, "sample item index exceed bound" );
            float bias;
            this->prepare_ifactor( tmp_ifactor, bias, feature );
            item_score[ idx ] = bias + cpu_only::dot( tmp_ufactor, tmp_ifactor );
        }
        inline void proc_rank( std::vector<int> &rst ){
            std::vector<Entry> entry;
            for( int i = 0; i < num_item_processed; i ++ ){
                if( item_tag[i] == svdranker_tag::BAN_SAMPLE  ) continue;
                item_score[ i ] += bias_ifactors[ i ] + cpu_only::dot( tmp_ufactor, tmp_ifactors[i] );
                entry.push_back( Entry( i, item_score[i] ) );                                 
            } 
            // sort the candidate
            std::sort( entry.begin(), entry.end() );
            if( top_k > 0 ){
                // grab result for top k
                apex_utils::assert_true( entry.size() >= static_cast<size_t>(top_k), "k can not exceed candidate size" );
                for( int k = 0; k < top_k; k ++ )
                    rst.push_back( entry[k].iid );
            }else{
                // grab rank positions for the pos samples
                for( size_t i = 0; i < entry.size(); i ++ ){
                    item_tag[ entry[i].iid ]  = static_cast<int>(i);
                }
                for( size_t i = 0; i < pos_item.size(); i ++ ){
                    rst.push_back( item_tag[ pos_item[i] ] );
                }
            }
        }
        inline void proc( std::vector<int> &rst, const SVDFeatureCSR::Elem &feature ){
            const int tag = static_cast<int>( feature.label );
            switch( tag ){
            case svdranker_tag::ITEM_TAG: proc_item( feature ); break;
            case svdranker_tag::USER_TAG: proc_user( feature ); break;
            case svdranker_tag::POS_SAMPLE: 
            case svdranker_tag::BAN_SAMPLE: proc_tag( feature, tag ); break;
            case svdranker_tag::SPEC_SAMPLE: proc_spec( feature ); break;
            case svdranker_tag::PROCESS_TAG:proc_rank( rst ); break;
            }
        }
    public:
        virtual void process( std::vector<int> &result, const SVDFeatureCSR::Elem &feature ){ 
            this->proc( result, feature );
        }
        virtual void process( std::vector<int> &result, const SVDPlusBlock &data ){
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                tmp_ufeedback = 0.0f;
                for( int i = 0; i < data.num_ufeedback; i ++ ){
                    const unsigned fid = data.index_ufeedback[i];
                    const float    val = data.value_ufeedback[i];
                    apex_utils::assert_true( fid < static_cast<unsigned>( model.param.num_ufeedback ), 
                                             "ufeedback id exceed bound" );
                    tmp_ufeedback += model.W_ufeedback[ fid ] * val;
                }            
            }
            for( int i = 0; i < data.data.num_row; i ++ ){
                this->proc( result, data.data[i] );
            }
        }        
    };
};

#endif
