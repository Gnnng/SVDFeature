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
#include <cstdio>
#include "../apex_svd_data.h"
#include "../apex-utils/apex_utils.h"

using namespace apex_svd;
const char *feedback = "NULL";

int main( int argc, char *argv[] ){
    if( argc < 3 ){
        printf("Usage:make_ugroup_buffer <feature_file> <output> [options...]\n"\
               "options: -scale_score scale_score -fd feedbackfile -max_block max_line\n"\
               "example: make_ugroup_buffer input output -fd feedback_file -scale_score 1\n"\
               "\tmake a buffer that same user is grouped together\n"\
               "\tinput file must group same user in consecutive lines\n"\
               "\tfeedback file provides implicit/explicit feedback information used by SVD++style model\n"\
               "\tno feedback file means no extra implicit/explicit information\n"\
               "\tscale_score will divide the score by scale_score, we suggest to scale the score to 0-1 if it's too big\n"\
               "\tmax_block specifies max number of lines of feature to be included in one storage block\n" \
               "\t  default max_block=10000, set it to smaller number to reduce the memory cost for input loading\n");
        return 0; 
    }
    IDataIterator<SVDPlusBlock> *loader = create_plus_iterator( input_type::TEXT_FEATURE );
    loader->set_param( "scale_score", "1.0" );
    
    time_t start = time( NULL );
    for( int i = 3; i < argc; i ++ ){
        if( !strcmp( argv[i], "-scale_score") ){
            loader->set_param( "scale_score", argv[++i] ); continue;
        }
        if( !strcmp( argv[i], "-fd") ){
            loader->set_param( "feedback_in", feedback = argv[++i] ); continue;
        }
        if( !strcmp( argv[i], "-max_block") ){
            loader->set_param( "block_max_line", argv[++i] ); continue;
        }
    }

    loader->set_param( "data_in", argv[1] );
    loader->init();

    printf("feature=%s,feedback=%s,start creating buffer...\n", argv[1], feedback );
    create_binary_buffer( argv[2], loader );
    printf("all generation end, %lu sec used\n", (unsigned long)(time(NULL) - start) );
    delete loader;
    return 0;
}
