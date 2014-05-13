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

#ifndef _APEX_TASK_H_
#define _APEX_TASK_H_
namespace apex_utils{
    /* interface of task program */
    class ITask{
    public:
        virtual void set_param( const char *name , const char *val ) = 0;
        virtual void set_task ( const char *task ) =0;
        virtual void print_task_help( FILE *fo ) const = 0;
        virtual void run_task( void )= 0;
    public:
        virtual ~ITask(){}
    };
    
    inline int run_task( int argc , char *argv[] , ITask *tsk ){
        if( argc < 2 ){
            tsk->print_task_help( stdout );
            return 0;            
        }   
        tsk->set_task( argv[1] );
        
        for( int i = 2 ; i < argc ; i ++ ){
            char name[256],val[256];
            if( sscanf( argv[i] ,"%[^=]=%[^\n]", name , val ) == 2 ){
                tsk->set_param( name , val );
		}   
        }
        tsk->run_task();
        return 0;
    } 
};
#endif
