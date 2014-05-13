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
#include "apex_svd.h"

// include all the useful customized-solvers
#include "solvers/base-solver/apex_svd_base.h"
#include "solvers/multi-imfb/apex_multi_imfb.h"
#include "solvers/bilinear/apex_svd_bilinear.h"
#include "solvers/gbrt/apex_gbrt.h"

namespace apex_svd{
    // return corresponding sub-solvers according to extend type
    ISVDTrainer *create_svd_trainer( SVDTypeParam mtype ){
        if( mtype.extend_type == 2  ) return new SVDPPMultiIMFB( mtype );
        if( mtype.extend_type == 15 ) return new SVDBiLinearTrainer( mtype );
        if( mtype.extend_type == 30 ) return new APLambdaGBRTTrainer( mtype );
        if( mtype.extend_type == 31 ) return new RegGBRTTrainer( mtype );
        // default solver 
        if( mtype.extend_type == 1  ) return new SVDPPFeature( mtype );
        if( mtype.format_type == svd_type::USER_GROUP_FORMAT ){ 
            return new apex_svd::SVDPPFeature( mtype );
        }else{
            return new apex_svd::SVDFeature( mtype );
        }
    }
    ISVDRanker *create_svd_ranker( SVDTypeParam mtype ){
        return new apex_svd::SVDFeatureRanker( mtype );
    }
};
