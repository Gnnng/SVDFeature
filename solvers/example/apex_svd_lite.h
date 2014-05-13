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
#ifndef _APEX_SVD_LITE_H_
#define _APEX_SVD_LITE_H_

#include "../../apex_svd.h"
#include <cstring>

namespace apex_svd{
    using namespace apex_tensor;
    using namespace apex_utils;
};

// this file is a lite version implementation of SVDFeature
// it's used as an example code for SGD implementation
// efficient SVD++ is not supported
namespace apex_svd{    
    class SVDFeatureLite: public ISVDTrainer{
    protected:
        int init_end;
        SVDModel model;
        SVDTrainParam param;
    private:
        CTensor1D tmp_ufactor, tmp_ifactor;
    public:
        SVDFeatureLite( const SVDTypeParam &mtype ){
            model.mtype = mtype;
            this->init_end = 0;
        }
        virtual ~SVDFeatureLite(){
            model.free_space();
            if( init_end == 0 ) return;
            tensor::free_space( tmp_ufactor );
            tensor::free_space( tmp_ifactor );
        }
    public:
        // model related interface
        virtual void set_param( const char *name, const char *val ){
            param.set_param( name, val );
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
            tmp_ufactor = clone( model.W_user[0] );
            tmp_ifactor = clone( model.W_item[0] );
            this->init_end = 1;
        }

        // do regularization
        inline void regularize( const SVDFeatureCSR::Elem &feature ){                        
            for( int i = 0; i < feature.num_global; i ++ ){                
                const unsigned gid = feature.index_global[i];
                model.g_bias[ gid ] *= ( 1.0f - param.learning_rate * param.wd_global );
            }
            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const unsigned uid = feature.index_ufactor[i];
                model.W_user[ uid ] *= ( 1.0f - param.learning_rate * param.wd_user ); 
                model.u_bias[ uid ] *= ( 1.0f - param.learning_rate * param.wd_user_bias );                
            }
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                model.W_item[ iid ] *= ( 1.0f - param.learning_rate * param.wd_item ); 
                model.i_bias[ iid ] *= ( 1.0f - param.learning_rate * param.wd_item_bias );
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
                }
            }
             
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                apex_utils::assert_true( iid < (unsigned)model.param.num_item, "item feature index exceed bound" );
                sum +=  ival * i_bias[ iid ];
            }
            
            return sum;
        }
        inline void prepare_tmp( const SVDFeatureCSR::Elem &feature ){ 
            tmp_ufactor = 0.0f;
            tmp_ifactor = 0.0f;

            for( int i = 0; i < feature.num_ufactor; i ++ ){
                const unsigned uid = feature.index_ufactor[i];
                apex_utils::assert_true( uid < (unsigned)model.param.num_user, "user feature index exceed bound" );
                tmp_ufactor += model.W_user[ uid ] * feature.value_ufactor[i];
            }            
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                tmp_ifactor += model.W_item[ iid ] * ival;
            }            
        }
                
        inline void update_no_decay( const SVDFeatureCSR::Elem &feature ){ 
            float err = active_type::cal_grad( feature.label, this->pred( feature ), model.mtype.active_type );

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
            }

            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
                float scale = param.learning_rate * err * ival;
                model.W_item[ iid ] += scale * tmp_ufactor;                 
                model.i_bias[ iid ] += scale;
            }                        
        }

    protected:
        inline float pred( const SVDFeatureCSR::Elem &feature ){ 
            double sum = model.param.base_score + 
                this->calc_bias( feature, model.u_bias, model.i_bias, model.g_bias );
            
            this->prepare_tmp( feature );

            sum += apex_tensor::cpu_only::dot( tmp_ufactor, tmp_ifactor );            
            
            return active_type::map_active( (float)sum, model.mtype.active_type ); 
        }
        inline void update_inner( const SVDFeatureCSR::Elem &feature ){ 
            this->update_no_decay( feature );
            this->regularize( feature );
        }        
    public:        
        virtual void update( const SVDFeatureCSR::Elem &feature ){             
            this->update_inner( feature );
        }
        virtual float predict( const SVDFeatureCSR::Elem &feature ){ 
            return this->pred( feature );
        }
        virtual void set_round( int nround ){
            // do nothing
        }
    };
};

#endif

