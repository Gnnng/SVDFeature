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
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <ctime>
#include <cstring>
#include <climits>
#include "apex_svd.h"
#include "apex-utils/apex_task.h"
#include "apex-utils/apex_utils.h"
#include "apex-utils/apex_config.h"
#include "apex-tensor/apex_random.h"

#include <fstream>
namespace apex_svd{

    class SVDInferTask : public apex_utils::ITask{
    private:
        // class for RMSE evaluation
        class RMSEEvaluator{
        private:
            long double sum_test, num_test;
        public:
            inline void init(){
                this->before_first();
            }
            inline void add_eval( float label, float pred, float scale = 1.0f ){
                double diff = (pred - label) * scale;
                sum_test += diff*diff;
                num_test += 1.0;            
            }
            inline void print_stat( FILE *fo, int iter ){
                fprintf( fo, "%d\t%lf\n", iter, sqrt( sum_test/num_test )  );
            }
            inline void before_first(){
                sum_test = 0.0f; num_test = 0.0f;
            }
        };    
    private:
        SVDTypeParam  mtype;
        ISVDRanker   *svd_ranker;
        ISVDTrainer  *svd_inferencer;
    private:
        RMSEEvaluator   rmse_eval;        
    private:
        int input_type;
        IDataIterator<SVDFeatureCSR::Elem> *itr_csr;
        IDataIterator<SVDPlusBlock>        *itr_plus;
    private:        
        float scale_score;
        int init_end, model_alloc;
        // name of job
        char name_job [ 256 ];
        char name_eval[ 256 ];
        // name of configure file
        char name_config[ 256 ];
        // name of prediction file to store
        char name_pred[ 256 ];
        // start counter of model
        int start, end;
        // id of model to be used as predictor
        int pred_model;
        // write prediction to binary file
        int pred_binary;
        // folder name of output  
        char name_model_in_folder[ 256 ];
        // sampling step
        int step;
        // use ranker to do ranking prediction
        int use_ranker, num_item_set;
        // silent
        int silent;
    private:
        apex_utils::ConfigSaver cfg;
    private:
        inline void reset_default(){
            this->step = 1;
            this->pred_model  = -1;
            this->pred_binary = 0; 
            this->scale_score = 1.0;
            init_end = 0; model_alloc = 0;
            this->itr_csr  = NULL;
            this->itr_plus = NULL;
            this->svd_ranker = NULL;
            this->svd_inferencer = NULL;
            strcpy( name_config, "config.conf" );
            strcpy( name_pred  , "pred.txt" );
            strcpy( name_model_in_folder, "models" );            
            strcpy( name_job, "" );
            strcpy( name_eval, "NULL" );       
            silent = 0; start = 0; end = INT_MAX; 
            use_ranker = 0; num_item_set = 0;
            this->input_type  = input_type::BINARY_BUFFER;
        }
    public:
        SVDInferTask(){
            this->reset_default();
        }
        virtual ~SVDInferTask(){
            if( init_end ){
                if( svd_inferencer != NULL ) delete svd_inferencer;
                if( svd_ranker != NULL ) delete svd_ranker;  
                if( itr_csr != NULL ) delete itr_csr;
                if( itr_plus!= NULL ) delete itr_plus; 
            }
        }
    private:
        inline void init_model( int start ){
            char name[256];
            sprintf(name,"%s/%04d.model" , name_model_in_folder, start );
            FILE *fi = apex_utils::fopen_check( name , "rb" );
            apex_utils::assert_true( fread( &mtype, sizeof(SVDTypeParam), 1, fi ) > 0, "load model" );
            if( use_ranker == 0 ){
                svd_inferencer = create_svd_trainer( mtype );
                svd_inferencer->load_model( fi );
                fclose( fi );
            }else{
                svd_ranker = create_svd_ranker( mtype );
                svd_ranker->load_model( fi );
                fclose( fi );
            }
        }
        inline bool load_model( int id ){
            char name[256];
            sprintf(name,"%s/%04d.model" , name_model_in_folder, id );
            FILE *fi = fopen64( name, "rb");
            if( fi == NULL ) return false;            
            apex_utils::assert_true( fread( &mtype, sizeof(SVDTypeParam), 1, fi ) > 0, "load model" );
            if( use_ranker == 0 ){
                svd_inferencer->load_model( fi );
            }else{
                svd_ranker->load_model( fi );
            }
            fclose( fi );
            return true;
        } 
        inline void set_param_inner( const char *name, const char *val ){
            if( !strcmp( name,"model_out_folder" ))   strcpy( name_model_in_folder, val ); 
            if( !strcmp( name,"log_eval" ))           strcpy( name_eval, val ); 
            if( !strcmp( name,"name_pred" ))          strcpy( name_pred, val ); 
            if( !strcmp( name, "start") )             start = atoi( val );
            if( !strcmp( name, "end") )               end = atoi( val );
            if( !strcmp( name, "focus") )             end = (start = atoi( val )) + 1;
            if( !strcmp( name, "pred") ){
                pred_model  = atoi( val );
                end = (start = atoi( val )) + 1;
            }
            if( !strcmp( name, "pred_binary") )       pred_binary  = atoi( val );
            if( !strcmp( name, "step") )              step = atoi( val );
            if( !strcmp( name, "silent") )            silent = atoi( val );
            if( !strcmp( name, "job") )               strcpy( name_job, val ); 
            if( !strcmp( name, "scale_score" ) )      scale_score = (float)atof( val );
            if( !strcmp( name, "test:input_type") )   input_type = atoi( val );
            if( !strcmp( name, "use_ranker") )        use_ranker = atoi( val ); 
            if( !strcmp( name, "num_item_set") )      num_item_set = atoi( val );           
        }
        
        inline void configure(){
            apex_utils::ConfigIterator itr( name_config );
            while( itr.next() ){
                cfg.push_back( itr.name(), itr.val() );
            }

            cfg.before_first();
            while( cfg.next() ){
                set_param_inner( cfg.name(), cfg.val() );
            }
            // decide format type
            mtype.decide_format( input_type == 2 ? svd_type::USER_GROUP_FORMAT : svd_type::AUTO_DETECT );
        }

        // configure iterator
        inline void configure_iterator( void ){
            if( mtype.format_type == svd_type::USER_GROUP_FORMAT ){
                this->itr_plus = create_plus_iterator( input_type );
            }else{
                this->itr_csr = create_csr_iterator( input_type );
            }        

            cfg.before_first();
            while( cfg.next() ){
                // only accept prefix "test:"
                if( !strncmp( cfg.name(), "test:", 5 ) ){
                    char name[ 256 ];
                    sscanf( cfg.name(), "test:%s", name );
                    if( itr_csr != NULL ) itr_csr->set_param ( name, cfg.val() );
                    if( itr_plus!= NULL ) itr_plus->set_param( name, cfg.val() );
                }
                // compatible issues
                if( !strcmp( cfg.name(), "data_test" )){
                    if( itr_csr != NULL ) itr_csr->set_param ( "data_in", cfg.val() );
                    if( itr_plus!= NULL ) itr_plus->set_param( "data_in", cfg.val() );
                }
                if( !strcmp( cfg.name(), "scale_score" )){
                    if( itr_csr != NULL ) itr_csr->set_param ( "scale_score", cfg.val() );
                    if( itr_plus!= NULL ) itr_plus->set_param( "scale_score", cfg.val() );
                }
                if( !strcmp( cfg.name(), "silent" )){
                    if( itr_csr != NULL ) itr_csr->set_param ( "silent", cfg.val() );
                    if( itr_plus!= NULL ) itr_plus->set_param( "silent", cfg.val() );
                }
            }
            if( itr_csr != NULL ) itr_csr->init();
            if( itr_plus!= NULL ) itr_plus->init();
        }

        inline void configure_inferencer(){
            cfg.before_first();
            while( cfg.next() ){
                if( svd_ranker != NULL ) svd_ranker->set_param( cfg.name(), cfg.val() );
                if( svd_inferencer != NULL ) svd_inferencer->set_param( cfg.name(), cfg.val() );
            }
        }
                
        inline void init( void ){
            rmse_eval.init();
            this->init_model( start );
            this->configure_inferencer();
            if( svd_inferencer != NULL ) svd_inferencer->init_trainer();
            if( svd_ranker != NULL ) svd_ranker->init_ranker( num_item_set );
            this->configure_iterator();
            this->init_end = 1;
        }     
     
        inline void task_eval(){
            FILE *fo = stdout;

            if( strcmp( name_eval, "NULL") ){
                fo = apex_utils::fopen_check( name_eval, "a" );
            }
            for( int iter = start; iter < end && this->load_model(iter); iter += step ){
                rmse_eval.before_first();                    
                
                if( itr_csr != NULL ){
                    itr_csr->before_first();
                    SVDFeatureCSR::Elem e;
                    while( itr_csr->next(e) ){                    
                        float p = svd_inferencer->predict( e );
                        rmse_eval.add_eval( e.label, p, scale_score );
                    }                    
                }
                if( itr_plus != NULL ){
                    SVDPlusBlock e;
                    itr_plus->before_first();
                    while( itr_plus->next(e) ){               
                        std::vector<float> p;
                        svd_inferencer->predict( p, e );
                        for( int i = 0; i < e.data.num_row; i ++ ){
                            rmse_eval.add_eval( e.data[i].label, p[i], scale_score );
                        }
                    }                    
                }
                rmse_eval.print_stat( fo, iter );
            }
            
            if( fo != stdout ){
                fclose( fo );
            }
        }
       
        inline FILE *fopen_pred() const{
            if( this->pred_binary == 0 ){
                return apex_utils::fopen_check( name_pred, "w" );
            }else{
                return apex_utils::fopen_check( name_pred, "wb" );                
            }
        }

        inline void write_pred( FILE *fo, float pred ) const{
            if( this->pred_binary == 0 ){
                fprintf( fo, "%f\n", pred );
            }else{
                fwrite( &pred, sizeof(float), 1, fo );
            }
        }

        inline void write_pred( FILE *fo, int pred ) const{
            if( this->pred_binary == 0 ){
                fprintf( fo, "%d\n", pred );
            }else{
                fwrite( &pred, sizeof(int), 1, fo );
            }
        }
        
        inline void task_pred(){
            apex_utils::assert_true( this->load_model(pred_model), "fail to load model" );
            if( !silent ) printf("start prediction...");
            FILE *fo = this->fopen_pred();
            if( itr_csr != NULL ){
                SVDFeatureCSR::Elem e;
                itr_csr->before_first();
				std::ofstream model_fout("model.txt");
                while( itr_csr->next( e ) ){
					model_fout
						<< e.label << "\t" //<< e.num_global << "\t" << e.num_ufactor << "\t" << e.num_ifactor << "\t"
						<< *(e.index_global) << "\t" << *(e.index_ufactor) << "\t" << *(e.index_ifactor) << "\t" << std::endl;
						//<< *(e.value_global) << "\t" << *(e.value_ufactor) << "\t" << *(e.value_ifactor) << std::endl;
					/*printf("e.")
					e.label = 
					e.num_global = row_ptr[r * 3 + 1] - row_ptr[r * 3 + 0];
					e.num_ufactor = row_ptr[r * 3 + 2] - row_ptr[r * 3 + 1];
					e.num_ifactor = row_ptr[r * 3 + 3] - row_ptr[r * 3 + 2];
					e.index_global = feat_index + row_ptr[r * 3 + 0];
					e.index_ufactor = feat_index + row_ptr[r * 3 + 1];
					e.index_ifactor = feat_index + row_ptr[r * 3 + 2];
					e.value_global = feat_value + row_ptr[r * 3 + 0];
					e.value_ufactor = feat_value + row_ptr[r * 3 + 1];
					e.value_ifactor = feat_value + row_ptr[r * 3 + 2];*/
                    float p = svd_inferencer->predict( e );
                    this->write_pred( fo, p * scale_score );
                }
				model_fout.close();
            }else{
                SVDPlusBlock e;
                itr_plus->before_first();
                while( itr_plus->next( e ) ){
                    std::vector<float> p;
                    svd_inferencer->predict( p, e );
                    for( size_t i = 0; i < p.size(); i ++ ) {
                        this->write_pred( fo, p[i] * scale_score );
                    }
                }                
            }

            fclose( fo );
            if( !silent ) printf("prediction end, results stored to %s\n",name_pred );
        }

        inline void task_pred_rank(){
            apex_utils::assert_true( this->load_model(pred_model), "fail to load model" );
            if( !silent ) printf("start prediction...");
            FILE *fo = this->fopen_pred();
            if( itr_csr != NULL ){
                SVDFeatureCSR::Elem e;
                itr_csr->before_first();
                while( itr_csr->next( e ) ){
                    std::vector<int> p;
                    svd_ranker->process( p, e );
                    for( size_t i = 0; i < p.size(); i ++ ) {
                        this->write_pred( fo, p[i] );
                    }
                }
            }else{
                SVDPlusBlock e;
                itr_plus->before_first();
                while( itr_plus->next( e ) ){
                    std::vector<int> p;
                    svd_ranker->process( p, e );
                    for( size_t i = 0; i < p.size(); i ++ ) {
                        this->write_pred( fo, p[i] );
                    }
                }                
            }

            fclose( fo );
            if( !silent ) printf("prediction end, results stored to %s\n",name_pred );
        }
                
    public:
        virtual void set_param( const char *name , const char *val ){
            cfg.push_back_high( name, val );
        }
        virtual void set_task ( const char *task ){
            strcpy( name_config, task );
        }
        virtual void print_task_help( FILE *fo ) const {
            printf("Usage:<config> [xxx=xx]\n");
        }
        virtual void run_task( void ){            
            this->configure();
            this->init();
            if( this->pred_model >= 0 ){                
                if( svd_inferencer != NULL ) this->task_pred();
                if( svd_ranker != NULL ) this->task_pred_rank();
            }else{
                apex_utils::assert_true( svd_inferencer != NULL, "can only use ranker for rank prediction" );
                this->task_eval();
            }
        }        
    };
};

int main( int argc, char *argv[] ){
    apex_random::seed( 10 );
    apex_svd::SVDInferTask tsk;
    return apex_utils::run_task( argc, argv, &tsk );
}

