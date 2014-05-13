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
#include "apex_svd_lite.h"

namespace apex_svd{
    //!! this is the key part of customization, use lite solver as main solver
    ISVDTrainer *create_svd_trainer( SVDTypeParam mtype ){
        // show me
        printf("SVDFeature:Lite\n");
        return new apex_svd::SVDFeatureLite( mtype );
    }
    ISVDRanker *create_svd_ranker( SVDTypeParam mtype ){
        return NULL;
    }
};
