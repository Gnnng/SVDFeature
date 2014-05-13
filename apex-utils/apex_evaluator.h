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
 * \file apex_evaluator.h
 * \brief this file defines the class for evalutation code
 * \author Tianqi Chen: tqchen@apex.sjtu.edu.cn
 */
#ifndef _APEX_EVAL_H_
#define _APEX_EVAL_H_

#include <cstdio>
#include <vector>
#include <algorithm>
#include "apex_utils.h"
#include "../apex-tensor/apex_random.h"

namespace apex_utils{    
    /*! \brief MAP evalutator, also support precision and recall */
    class EvaluatorMAP{
    private:
        int round;
        long ninst;
        double sum_mapall;
        // MAP
        std::vector<double> sum_map;
        std::vector<int>    map_ats;
        // precision
        std::vector<int>    pre_ats;
        std::vector<double> sum_pre;
        // recall
        std::vector<int>    rec_ats;
        std::vector<double> sum_rec;
    private:
        static inline bool cmp_score( const std::pair<float,int> &a, const std::pair<float,int> &b ){
            return a.first > b.first;
        }
    public:
        /*! \brief constructor */
        EvaluatorMAP( void ){
            this->init();
        }        
        /*! 
         * \brief add k of MAP@k 
         * \param k the k of MAP@k
         */
        inline void add_map_at( int k ){
            apex_utils::assert_true( ninst == 0, "can not add at during evalution" );
            map_ats.push_back( k );
            sum_map.push_back( 0.0 );
        }
        /*! 
         * \brief add k of Pre@k 
         * \param k the k of Pre@k
         */
        inline void add_precision_at( int k ){
            apex_utils::assert_true( ninst == 0, "can not add at during evalution" );
            pre_ats.push_back( k );
            sum_pre.push_back( 0.0 );
        }
        /*! 
         * \brief add k of Rec@k 
         * \param k the k of Rec@k
         */
        inline void add_recall_at( int k ){
            apex_utils::assert_true( ninst == 0, "can not add at during evalution" );
            rec_ats.push_back( k );
            sum_rec.push_back( 0.0 );
        }
        /*! 
         * \brief add a evaluator setting
         * \param setstr a string that describes the setting
         * \return whether a new setting is succesfully added
         */
        inline bool add_setting( const char *setstr ){
            int k;
            if( sscanf( setstr, "MAP@%d", &k ) == 1 ){
                this->add_map_at( k ); return true;
            }
            if( sscanf( setstr, "map@%d", &k ) == 1 ){
                this->add_map_at( k ); return true;
            }
            if( sscanf( setstr, "P@%d", &k ) == 1 ){
                this->add_precision_at( k ); return true;
            }
            if( sscanf( setstr, "Pre@%d", &k ) == 1 ){
                this->add_precision_at( k ); return true;
            }
            if( sscanf( setstr, "precision@%d", &k ) == 1 ){
                this->add_precision_at( k ); return true;
            }
            if( sscanf( setstr, "Rec@%d", &k ) == 1 ){
                this->add_recall_at( k ); return true;
            }
            if( sscanf( setstr, "recall@%d", &k ) == 1 ){
                this->add_recall_at( k ); return true;
            }
            return false;
        }
        /*! 
         * \brief initialize the evaluator
         * \param round round number which can used to shown on print, can be -1 which will not be displayed
         */
        inline void init( int round = -1 ){
            this->round = round;
            ninst = 0;
            sum_mapall = 0.0;
            std::fill( sum_map.begin(), sum_map.end(), 0.0 );
            std::fill( sum_pre.begin(), sum_pre.end(), 0.0 );
            std::fill( sum_rec.begin(), sum_rec.end(), 0.0 );
            std::sort( map_ats.begin(), map_ats.end() );
            std::sort( pre_ats.begin(), pre_ats.end() );
            std::sort( rec_ats.begin(), rec_ats.end() );
        }
        /*! 
         * \brief update evaluator using a record
         * \param pos_idx gives the rank position of all positive samples, rank position starts from 0
         * \param is_sorted whether pos_idx is sorted, if not sorted, the function will sort it first
         */
        inline void add_eval( std::vector< int > &pos_idx, bool is_sorted = false ){
            if( !is_sorted ) std::sort( pos_idx.begin(), pos_idx.end() );
            apex_utils::assert_true( pos_idx.size() != 0, "no positive record presented" );
            {// MAP
                double sumap = 0.0;
                std::vector<float> sum_ar;
                for( size_t i = 0; i < pos_idx.size(); i ++ ){
                    double pp = ((double)(i+1)) / ( pos_idx[i] + 1 );
                    sumap  += pp;
                    sum_ar.push_back( sumap );
                }
                sum_mapall += sumap / pos_idx.size();

                size_t j = 0;
                for( size_t i = 0; i < map_ats.size(); i ++ ){
                    while( j < pos_idx.size() && pos_idx[j] < map_ats[i] ) j ++;                   
                    if( j != 0 ){
                        sum_map[ i ] += sum_ar[ j - 1 ] / pos_idx.size();
                    }
                }
            }
            {// precision
                size_t j = 0;
                for( size_t i = 0; i < pre_ats.size(); i ++ ){
                    while( j < pos_idx.size() && pos_idx[j] < pre_ats[i] ) j ++;
                    sum_pre[ i ] += ( (double)j )/ pre_ats[i];
                }
            }
            {// recall
                size_t j = 0;
                for( size_t i = 0; i < rec_ats.size(); i ++ ){
                    while( j < pos_idx.size() && pos_idx[j] < rec_ats[i] ) j ++;
                    sum_rec[ i ] += ( (double)j )/ pos_idx.size();
                }
            }
            ninst ++;
        }
        /*! 
         * \brief update evaluator using a record
         * \param rec defines the ranked list, not needed to be sorted, will be sorted in the function
         *  rec[i].first defines is rank score,          
         *  rec[i].second defines whether current instance is positive, 
         *  when rec[i].second > 0, then the list 
         */
        inline void add_eval( std::vector< std::pair<float, int> > &rec ){
            apex_random::shuffle( rec);
            std::sort( rec.begin(), rec.end(), cmp_score );

            std::vector<int> pos_idx;
            for( size_t i = 0; i < rec.size(); i ++ ){
                if( rec[i].second > 0 ){
                    pos_idx.push_back( (int)i );
                }
            }
            this->add_eval( pos_idx, true );
        }
        /*! 
         * \brief print evaluated result  
         * \param fo output stream
         * \param newline whether to start a new line during writing
         */
        inline void print_result( FILE *fo = stdout, bool newline = true ){
            if( round >= 0 ) fprintf( fo, "%d\t", round );
            fprintf( fo, "MAP:%f", sum_mapall / ninst );
            for( size_t i = 0; i < map_ats.size(); i ++ ){
               fprintf( fo," MAP@%d:%f", map_ats[i], sum_map[i] / ninst );
            }
            for( size_t i = 0; i < pre_ats.size(); i ++ ){
               fprintf( fo," Pre@%d:%f", pre_ats[i], sum_pre[i] / ninst );
            }
            for( size_t i = 0; i < rec_ats.size(); i ++ ){
               fprintf( fo," Rec@%d:%f", rec_ats[i], sum_rec[i] / ninst );
            }
            if( newline ) fprintf( fo, "\n" );
        }
        /*! 
         * \brief return current number of instances
         * return current number of instances
         */
        inline long num_instance( void ) const{
            return ninst;
        }
    };
};

#endif
