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
 * \file apex_multi_imfb.h
 * \brief SVDFeature implementation of Local Implicit Feedback Model 
 *  publication: Yang et al. Local Implicit Feedback Mining for Music Recommendation. RecSys 2012        
 * \author Tianqi Chen, Diyi Yang: {tqchen,yangdiyi}@apex.sjtu.edu.cn
 */
#ifndef _APEX_MULTI_IMFB_H_
#define _APEX_MULTI_IMFB_H_

#include "../../apex_svd.h"
#include <cstring>

namespace apex_svd{
    // SVD++ plugin for SVDFeature
    class SVDPPMultiIMFB: public apex_svd::SVDFeature{
    private:
        struct IMFBEntry{
            int num_ufeedback;
            float norm_ufeedback;
            float tmp_ufeedback_bias, old_ufeedback_bias;
            CTensor1D tmp_ufeedback, old_ufeedback;
        };
        size_t top;
        // disable the top levels that occurs in the model
        std::vector<bool> disable_level;
        std::vector<IMFBEntry> imfb;        
    public:
        SVDPPMultiIMFB( const SVDTypeParam &mtype ):
            SVDFeature( mtype ){
            this->top = 0;
        }
        virtual ~SVDPPMultiIMFB(){
            for( size_t i = 0; i < imfb.size(); i ++ ){
                tensor::free_space( imfb[i].tmp_ufeedback );
                tensor::free_space( imfb[i].old_ufeedback );            
            }            
        }
        virtual void set_param( const char *name, const char *val ){
            if( !strcmp( name,"ufeedback_disable_level" )) {
                int level = atoi( val );
                while( disable_level.size() <= static_cast<size_t>( level ) ){
                    disable_level.push_back( false );
                }
                disable_level[ level ] =  true;
            }
            SVDFeature::set_param( name, val );
        }
    protected:
        // implicit feedback information
        virtual void prepare_svdpp( CTensor1D &tmp_ufactor ){
            if( top == 0 ){
                tmp_ufactor = 0.0f; 
            }else{
                tensor::copy( tmp_ufactor, imfb[0].tmp_ufeedback ); 
            }
            for( size_t i = 1; i < top; i ++ ){
                tmp_ufactor += imfb[i].tmp_ufeedback;
            }
        }
        virtual float get_bias_svdpp( void ){
            float sum = 0.0f;
            for( size_t i = 0; i < top; i ++ ){
                sum += imfb[i].tmp_ufeedback_bias;
            }
            return sum;
        }               
        virtual void update_svdpp( float err, const CTensor1D &tmp_ifactor ){
            float lr = param.learning_rate * param.scale_lr_ufeedback;
            for( size_t i = 0; i < top; i ++  ){
                if( disable_level[i] || imfb[i].num_ufeedback == 0 ) continue;
                imfb[i].tmp_ufeedback += lr * err * imfb[i].norm_ufeedback * tmp_ifactor;
                imfb[i].tmp_ufeedback *= ( 1.0f - lr * param.wd_ufeedback );
                if( model.param.no_user_bias == 0 ){
                    imfb[i].tmp_ufeedback_bias += lr * err * imfb[i].norm_ufeedback;
                    imfb[i].tmp_ufeedback_bias *= ( 1.0f  - lr * param.wd_ufeedback_bias );
                }            
            }
        }
        virtual void remove_svdpp( CTensor1D &tmp_ufactor, unsigned fid, float fval ){
            apex_utils::assert_true( fid < static_cast<unsigned>( model.param.num_ufeedback ), 
                                     "ufeedback id exceed bound" );
            tmp_ufactor += model.W_ufeedback[ fid ] * fval;
        }
        virtual float remove_svdpp_bias( unsigned fid, float fval ){
            apex_utils::assert_true( fid < static_cast<unsigned>( model.param.num_ufeedback ), 
                                     "ufeedback id exceed bound" );
            return model.ufeedback_bias[ fid ] * fval;
            
        }
        virtual void update_rmsvdpp( float err, const CTensor1D &tmp_ifactor, unsigned fid, float fval ){
            float lr = param.learning_rate * param.scale_lr_ufeedback;
            model.W_ufeedback[ fid ] += lr * err * fval * tmp_ifactor;
            // remove effect of regularization
            if( model.param.no_user_bias == 0 ){
                model.ufeedback_bias[ fid ] += lr * err * fval;
            }
        }
    private:
        // SVD++ style
        inline void prepare_ufeedback( IMFBEntry &e, const SVDPlusBlock &data ){ 
            e.norm_ufeedback = 0.0f;
            e.tmp_ufeedback = 0.0f; e.tmp_ufeedback_bias = 0.0f;
            for( int i = 0; i < data.num_ufeedback; i ++ ){
                const unsigned fid = data.index_ufeedback[i];
                const float    val = data.value_ufeedback[i];
                apex_utils::assert_true( fid < static_cast<unsigned>( model.param.num_ufeedback ), 
                                         "ufeedback id exceed bound" );
                e.tmp_ufeedback += model.W_ufeedback[ fid ] * val;
                e.norm_ufeedback+= val * val;
                if( model.param.no_user_bias == 0 ){
                    e.tmp_ufeedback_bias += model.ufeedback_bias[ fid ] * val;
                }
            }
            e.old_ufeedback_bias = e.tmp_ufeedback_bias;
            e.num_ufeedback = data.num_ufeedback;
            tensor::copy( e.old_ufeedback, e.tmp_ufeedback );                        
        }        
        inline void update_ufeedback( IMFBEntry &e, const SVDPlusBlock &data ){ 
            if( data.num_ufeedback == 0 ) return;
            e.tmp_ufeedback -= e.old_ufeedback;
            e.tmp_ufeedback_bias -= e.old_ufeedback_bias;
            e.tmp_ufeedback *= 1.0f/ e.norm_ufeedback;
            e.tmp_ufeedback_bias *= 1.0f / e.norm_ufeedback;
            for( int i = 0; i < data.num_ufeedback; i ++ ){
                const unsigned fid = data.index_ufeedback[i];
                const float    val = data.value_ufeedback[i];
                model.W_ufeedback[ fid ] += e.tmp_ufeedback * val;
                if( model.param.no_user_bias == 0 ){
                    model.ufeedback_bias[ fid ] += e.tmp_ufeedback_bias * val;
                }
            }
        }
        inline void push_ufeedback( const SVDPlusBlock &data ){
            if( top == imfb.size() ){
                imfb.push_back( IMFBEntry() );
                imfb.back().tmp_ufeedback = clone( model.W_user[0] ); 
                imfb.back().old_ufeedback = clone( model.W_user[0] ); 
            }
            this->prepare_ufeedback( imfb[ top++ ] , data );
            // make sure disable level follows ok
            while( top > disable_level.size() ){
                disable_level.push_back( false );
            }
        }
    public:
        // SVD++ style
        virtual void update( const SVDPlusBlock &data ){ 
            // calculate and backup feedback in the first block
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                this->push_ufeedback( data );
            }
            // update 
            for( int i = 0; i < data.data.num_row; i ++ ){
                this->update_inner( data.data[i] );
            }
            // update feedback data in the last block
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::END_TAG ){
                apex_utils::assert_true( top != 0, "start tag,end tag error in implicit feedback" );
                -- top;
                if( !disable_level[ top ] ){
                    this->update_ufeedback( imfb[ top ], data );
                }
            }
        }
        virtual void predict( std::vector<float> &p, const SVDPlusBlock &data ){ 
            p.clear();
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::START_TAG ){
                this->push_ufeedback( data );
            }
            for( int i = 0; i < data.data.num_row; i ++ ){
                p.push_back( this->pred( data.data[i] ) );
            }
            if( data.extend_tag == svdpp_tag::DEFAULT || data.extend_tag == svdpp_tag::END_TAG ){
                apex_utils::assert_true( top != 0, "start tag,end tag error in implicit feedback" );
                -- top;
            }
        }        
    };
};

#endif
