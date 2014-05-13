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
 * \file apex_svd_bilinear.h
 * \brief SVDFeature:BiLinear, allow part of user feedback to interact with item feature to form bilinear model
 * \author Tianqi Chen: tqchen@apex.sjtu.edu.cn
 */
#ifndef _APEX_SVD_BILINEAR_H_
#define _APEX_SVD_BILINEAR_H_

// lambda rank trial 
namespace apex_svd{
    // template class of bilinear form trainer
    class SVDBiLinearTrainer : public SVDPPFeature{ 
    private:
        struct BParam{
            int num_bi_feedback;
            int start_ufeedback;
            int reserved[ 32 ];
            BParam( void ){
                start_ufeedback = 0;
                num_bi_feedback = 0;
                memset( reserved, 0, sizeof( reserved ) );
            }
            inline void set_param( const char *name, const char *val ){
                if( !strcmp( name, "num_bi_feedback") ) num_bi_feedback = atoi( val );
                if( !strcmp( name, "start_ufeedback") ) start_ufeedback = atoi( val );
            }
        };
            
        struct BModel{
            BParam param;
            CTensor2D W_bi;
            int space_allocated;
            BModel( void ){
                space_allocated = 0;
            }
            inline void alloc_space( int num_item ){
                W_bi.set_param( num_item, param.num_bi_feedback );
                apex_tensor::tensor::alloc_space( W_bi );
                W_bi = 0.0f;
                space_allocated = 1;
            }
            inline void free_space( void ){
                if( space_allocated == 0 ) return;
                apex_tensor::tensor::free_space( W_bi );
                space_allocated = 0;
            }
            inline void save_to_file( FILE *fo ){
                fwrite( &param, sizeof( BParam ), 1, fo ); 
                apex_tensor::cpu_only::save_to_file( W_bi, fo ); 
            }
            inline void load_from_file( FILE * fi, int num_item ){
                apex_utils::assert_true( fread( &param, sizeof( BParam ), 1, fi ) > 0, "load from file" );
                if( space_allocated == 0 ) this->alloc_space( num_item );
                apex_tensor::cpu_only::load_from_file( W_bi, fi, true );
            } 
        };
        
        BModel bmodel;
        int reg_bi_feedback;
        float wd_bi_feedback;
        float slr_bi_feedback;
        // user property index and value
        std::vector<unsigned> up_index;
        std::vector<float>    up_value;
        std::vector<unsigned> ref_bi;
    public:
        SVDBiLinearTrainer( const SVDTypeParam &mtype ):
            SVDPPFeature( mtype ){
            reg_bi_feedback = 0;
            wd_bi_feedback = 0.0f;
            slr_bi_feedback = 1.0f;
        }
        virtual ~SVDBiLinearTrainer( void ){
            bmodel.free_space();
        }
    private:
        inline void reg_feedback( float lr, unsigned iid, unsigned fid ){
            float lambda = lr * wd_bi_feedback;
            switch( reg_bi_feedback ){
            case 0: bmodel.W_bi[ iid ][ fid ] *= ( 1.0f - lambda ); break;
            case 1: reg_L1( bmodel.W_bi[ iid ][ fid ], lambda );    break;
            case 2: 
            case 3: break;
            case 4:                
            case 5: {// lazy L1 decay, truncate gradient
                unsigned rid = (unsigned)( iid * bmodel.param.num_bi_feedback + fid );
                float k = static_cast<float>( ref_bi[ rid ] - sample_counter ) + 1.0;                
                ref_bi[ rid ] = sample_counter;
                reg_L1( bmodel.W_bi[ iid ][ fid ], lambda * k ); 
                break;
            }
            default:
                apex_utils::error("unknown bi feedback decay method");
            }
        }
        inline void reg_feedback( float lr, unsigned iid ){
            float lambda = lr * wd_bi_feedback;
            switch( reg_bi_feedback ){
            case 0:
            case 1:
            case 4:
            case 5: break;
            case 2: bmodel.W_bi[ iid ] *= ( 1.0f - lambda );  break;
            case 3:{
                CTensor1D W_reg;
                W_reg = bmodel.W_bi[ iid ];
                tensor::regularize_L1( W_reg, lambda ); break;
            }
            default:
                apex_utils::error("unknown bi feedback decay method");
            }			
        }
		inline void add_bias_plugin( float &sum, unsigned iid, float ival ){
			for( size_t j = 0; j < up_index.size(); j ++ ){
				sum += bmodel.W_bi[ iid ][ up_index[j] ] * ival * up_value[j];                    
			}
		}
		inline void update_bias_plugin( float err, float lr, unsigned iid, float ival ){
			for( size_t j = 0; j < up_index.size(); j ++ ){
				bmodel.W_bi[ iid ][ up_index[j] ] += err * ival * up_value[j] * lr;
				this->reg_feedback( lr, iid, up_index[j] );
			}
		}
    protected:
        virtual float get_bias_plugin( const SVDFeatureCSR::Elem &feature ){
            float sum = 0.0f;
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
				this->add_bias_plugin( sum, iid, ival );

                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
					this->add_bias_plugin( sum, vec[j].index, vec[j].value * ival );
                }
            }
            return sum;
        }
        virtual void update_bias_plugin( float err, const SVDFeatureCSR::Elem &feature ){
            float lr = param.learning_rate * slr_bi_feedback; 
            for( int i = 0; i < feature.num_ifactor; i ++ ){
                const unsigned iid = feature.index_ifactor[i];
                const float    ival= feature.value_ifactor[i];
				this->update_bias_plugin( err, lr, iid, ival );
				
                SparseFeatureArray<float>::Vector vec = feat_item[ iid ];
                for( int j = 0; j < vec.size(); j ++ ){
					this->update_bias_plugin( err, lr, vec[j].index, vec[j].value * ival );
                }				
                this->reg_feedback( lr, iid );
            }            
        }        
    protected:
        virtual void prepare_ufeedback( const SVDPlusBlock &data, unsigned start_fid = 0 ){ 
            up_index.resize( 0 ); up_value.resize( 0 );
            SVDPPFeature::prepare_ufeedback( data, (unsigned)bmodel.param.start_ufeedback );
            for( int i = 0; i < data.num_ufeedback; i ++ ){                
                const unsigned fid = data.index_ufeedback[i];
                const float    val = data.value_ufeedback[i];
                if( fid < (unsigned)bmodel.param.num_bi_feedback ){
                    up_index.push_back( fid );
                    up_value.push_back( val );
                }
            }
        }
        virtual void update_ufeedback( const SVDPlusBlock &data, unsigned start_fid = 0 ){ 
            SVDPPFeature::update_ufeedback( data, (unsigned)bmodel.param.start_ufeedback );            
        }
    public:
        virtual void set_param( const char *name, const char *val ){
            SVDPPFeature::set_param( name, val );            
            if( !strcmp( name, "reg_bi_feedback") ) reg_bi_feedback = atoi( val );
            if( !strcmp( name, "slr_bi_feedback") ) slr_bi_feedback = (float)atof( val );
            if( !strcmp( name, "wd_bi_feedback") )  wd_bi_feedback  = (float)atof( val );
            if( bmodel.space_allocated == 0 ) bmodel.param.set_param( name, val );
        }
        virtual void load_model( FILE *fi ) {
            SVDPPFeature::load_model( fi );
            bmodel.load_from_file( fi, model.param.num_item );
        }
        virtual void save_model( FILE *fo ) {
            SVDPPFeature::save_model( fo );
            bmodel.save_to_file( fo );
        }
        virtual void init_model( void ){
            SVDPPFeature::init_model();
            bmodel.alloc_space( model.param.num_item );
        }
        virtual void init_trainer( void ){
            SVDPPFeature::init_trainer();
            if( reg_bi_feedback >= 4 ){
                ref_bi.resize( bmodel.param.num_bi_feedback * model.param.num_item );
                std::fill( ref_bi.begin(), ref_bi.end(), 0 );
            }
        }
    };
};

#endif
