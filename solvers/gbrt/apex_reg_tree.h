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

#ifndef _APEX_REG_TREE_H_
#define _APEX_REG_TREE_H_

/*!
 * \file apex_reg_tree.h
 * \brief header defining the interface of regression tree
 * \author Tianqi Chen, Xuezhi Cao: {tqchen,cxz}@apex.sjtu.edu.cn 
 */
#include <vector>
#include <climits>
#include "../../apex-utils/apex_utils.h"

/*! \brief namespace of regression tree */
namespace apex_rt{
    /*! \brief interger type used in RT */
    typedef int rt_int;
    /*! \brief unsigned interger type used in RT */
    typedef unsigned rt_uint;
    /*! \brief float type used in RT */
    typedef float rt_float;    
    /*! \brief whether to use debug mode */
    const bool rt_debug = true;
};

namespace apex_rt{
    /*! \brief feature vector of a instance, dense format, it's a dense array and use trick to represent unkown */
    class FVector{
    private:
        /*! \brief pointer to the data */
        rt_float *dptr;
        /*! \brief size of the data */
        rt_int    len;
    public:
        /*! \brief set the inner state parameters  */
        inline void set_state( rt_float *dptr, rt_int len ){
            this->dptr = dptr;
            this->len  = len;
        }
        /*! \brief size of feature vector */
        inline rt_int size( void ) const{
            return len;
        }
        /*! \brief get idx element */
        inline rt_float &operator[]( int idx ){
            return dptr[ idx ];
        }
        /*! \brief get idx element */
        inline const rt_float &operator[]( int idx ) const{
            return dptr[ idx ];
        }
        /*! \brief check whether idx's element is unknown */
        inline bool is_unknown( int idx ) const{
            return  *((int*)(dptr + idx)) == -1;
        }
        /*! \brief set idx's element to unknown */
        inline void set_unknown( int idx ){
            *((int*)(dptr + idx)) = -1;
        }
    };

    /*! \brief Sparse format of feature vector */
    struct FVectorSparse{
        /*! \brief array of feature index */
        rt_uint  *findex;
        /*! \brief array of feature value */
        rt_float *fvalue;
        /*! \brief size of the data */
        rt_int    len;        
    };
    
    /*! \brief feature matrix of all the instances */
    class FMatrix{
    private:
        /*! \brief dense data storage */
        std::vector<rt_float> data;
        /*! \brief sparse data index of the data */
        std::vector<int> sparse_idx;
        /*! \brief row pointer of CSR sparse storage */
        std::vector<unsigned>  row_ptr;
        /*! \brief index of CSR format */
        std::vector<rt_uint>   findex;
        /*! \brief value of CSR format */
        std::vector<rt_float>  fvalue;        
        /*! \brief length of column */
        rt_int len_col;
    public:
        /*! \brief constructor  */
        FMatrix( void ){ this->clear(); }
        /*! 
         * \brief get number of rows 
         * \return number of rows
         */
        inline size_t num_row( void ) const{
            return sparse_idx.size();
        }
        /*! 
         * \brief set the length of column  
         * \param len_col column length
         */
        inline void set_column_len( rt_int len_col ){
            apex_utils::assert_true( num_row() == 0, "can only set column length when num_row=0" );
            this->len_col = len_col;
        }
        /*! \brief clear the storage */
        inline void clear( void ){
            data.resize( 0 );
            sparse_idx.resize( 0 );
            row_ptr.resize( 0 );
            findex.resize( 0 );
            fvalue.resize( 0 );
            row_ptr.push_back( 0 );
        }
        /*! \brief add sparse part to the storage 
         *  \param feat sparse feature
         *  \param fstart start bound of feature
         *  \param fend   end bound range of feature
         *  \return the sparse index of this sparse feature
         */
        inline int add_spart( FVectorSparse feat, unsigned fstart = 0, unsigned fend = UINT_MAX ){
            apex_utils::assert_true( feat.len >= 0, "sparse feature length can not be negative" );
            unsigned cnt = 0;
            for( int i = 0; i < feat.len; i ++ ){
                if( feat.findex[i] < fstart || feat.findex[i] >= fend ) continue;
                findex.push_back( feat.findex[i] );
                fvalue.push_back( feat.fvalue[i] );
                cnt ++;
            }
            row_ptr.push_back( row_ptr.back() + cnt );
            return row_ptr.size() - 2;
        }
        /*! 
         * \brief add one row to the FMatrix 
         * \param sidx sparse index part of this instance, returns -1 if there is no sparse part
         * \return the row added
         */
        inline size_t add_row( int sidx = -1 ){
            sparse_idx.push_back( sidx );
            data.resize( num_row() * static_cast<size_t>(len_col) );
            apex_utils::assert_true( sidx < (int)row_ptr.size(), "sidx exceed bound");
            return num_row() - 1;
        }
        /*! \brief get feature vector of specific row */
        inline FVector operator[]( size_t row_id ){
            FVector v;
            apex_utils::assert_true( !rt_debug || row_id < num_row(), "row id exceed bound" );
            v.set_state( &data[ row_id * len_col ], len_col );
            return v;
        }
        /*! \brief get sparse part of current row */
        inline FVectorSparse get_spart( size_t row_id ){
            FVectorSparse sp;
            apex_utils::assert_true( !rt_debug || row_id < num_row(), "row id exceed bound" );
            int sidx = sparse_idx[ row_id ];
            if( sidx == -1 ){
                sp.len = 0; return sp;
            }
            sp.len = row_ptr[ sidx + 1 ] - row_ptr[ sidx ];
            sp.findex = &findex[ row_ptr[ sidx ] ];
            sp.fvalue = &fvalue[ row_ptr[ sidx ] ];
            return sp;
        }
    };
    
    /*! \brief auxlilary feature matrix to store sparse instance */
    class FMatrixS{        
    private:
        /*! \brief row pointer of CSR sparse storage */
        std::vector<size_t>  row_ptr;
        /*! \brief index of CSR format */
        std::vector<rt_uint>   findex;
        /*! \brief value of CSR format */
        std::vector<rt_float>  fvalue;
    public:
        /*! \brief constructor */
        FMatrixS( void ){ this->clear(); }
        /*! 
         * \brief get number of rows 
         * \return number of rows
         */
        inline size_t num_row( void ) const{
            return row_ptr.size() - 1;
        }
        /*! \brief clear the storage */
        inline void clear( void ){
            row_ptr.resize( 0 );
            findex.resize( 0 );
            fvalue.resize( 0 );
            row_ptr.push_back( 0 );
        }
        /*! 
         * \brief add a row to the matrix
         *  \param feat sparse feature
         *  \return the row id addted
         */
        inline size_t add_row( const FVectorSparse &feat ){
            apex_utils::assert_true( feat.len >= 0, "sparse feature length can not be negative" );
            row_ptr.push_back( row_ptr.back() + feat.len );
            for( int i = 0; i < feat.len; i ++ ){
                findex.push_back( feat.findex[i] );
                fvalue.push_back( feat.fvalue[i] );
            }
            return row_ptr.size() - 2;
        }
        /*! \brief get sparse part of current row */
        inline FVectorSparse operator[]( size_t sidx ){
            FVectorSparse sp;
            apex_utils::assert_true( !rt_debug || sidx < num_row(), "row id exceed bound" );
            sp.len = row_ptr[ sidx + 1 ] - row_ptr[ sidx ];
            sp.findex = &findex[ row_ptr[ sidx ] ];
            sp.fvalue = &fvalue[ row_ptr[ sidx ] ];
            return sp;
        }
    };

    /*! \brief interface of single regression trainer */
    class IRTTrainer{
    public:
        // interface for model setting and loading
        // calling pattern:
        //
        //   trainer->set_param for setting necessary parameters
        //   if new model to be trained, trainer->init_trainer
        //   elseif just to load from file, trainer->load_model
        //   trainer->do_boost
        //   trainer->save_model
        /*! 
         * \brief set parameters from outside 
         * \param name name of the parameter
         * \param val  value of the parameter
         */
        virtual void set_param( const char *name, const char *val ) = 0;
        /*! 
         * \brief load model from file
         * \param fi file pointer to input file
         */        
        virtual void load_model( FILE *fi ) = 0;
        /*! 
         * \brief save model file
         * \param fo file pointer to output file
         */
        virtual void save_model( FILE *fo ) const = 0;
        /*!
         * \brief initialize solver before training, called before training
         * this function is reserved for solver to allocate necessary space and 
         * do other preparations 
         */        
        virtual void init_trainer( void ) = 0;
    public:
        /*! 
         * \brief do gradient boost training for one step, using the information given
         * \param grad first order gradient of each instance
         * \param grad_second second order gradient of each instance
         * \param mat feature matrix of all the instances
         * \param group_id pre-partitioned group id of each instance, can be empty which indicates that no pre-partition involved
         * \param weight weight of each instance, can be empty which means weight=1 for all instances
         * \param smat sparse feature matrix of additional features in sparse format, is optional, can pass a num_row()=0 smat 
         *             which indicates no extra sparse feature is added
         */
        virtual void do_boost( std::vector<float> &grad, 
                               std::vector<float> &grad_second,
                               FMatrix &mat,
                               std::vector<unsigned> &group_id,
                               std::vector<float>    &weight,
                               FMatrixS &smat ) = 0;        
        /*! 
         * \brief predict values for given feature, and common user feature,
         *        a preliminary implementation handles the case where fcommon is empty and gid = 0
         * \param feat vector of input features
         * \param fcommon vector of common features, can be length 0 to mean no common feature involved
         * \param gid group id of current instance, default = 0
         * \return prediction 
         */        
        virtual float predict( const FVector &feat, const FVector &fcommon, unsigned gid = 0 ) = 0;

        /*! 
         * \brief get the leaf id given current feature
         *        a preliminary implementation handles the case where fcommon is empty and gid = 0
         * \param feat vector of input features
         * \param fcommon vector of common features, can be length 0 to mean no common feature involved
         * \param gid group id of current instance, default = 0
         * \return leaf id
         * \sa predict
         */        
        virtual int get_leaf_id( const FVector &feat, const FVector &fcommon, unsigned gid = 0 ) = 0;
        
        /*! 
         * \brief statistics about the tree, not necessary implemented
         * \param max_depth maximum depth of the tree
         * \param num_roots number of roots
         * \param num_nodes number of nodes besides the root
         */        
        virtual void get_stats( int &max_depth, int &num_roots, int &num_nodes ) const{ apex_utils::error("not implemented"); }
    public:
        virtual ~IRTTrainer( void ){}
    };
};

namespace apex_rt{
    /*! 
     * \brief returns a rt trainer 
     * \param rt_type indicate which implementation to be used
     * \return the trainer created
     */
    IRTTrainer *create_rt_trainer( int rt_type );
};
#endif
