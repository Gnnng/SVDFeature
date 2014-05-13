#!/bin/bash

# this is a demo script for training the movielen 
# get data if it doesn't exist

if [[ -f ua.base && -f ua.test ]]
then
    echo "start demo using movielen data"
else
    echo "The demo require input file ua.base, ua.test from ml-100K in current folder, please get them from www.grouplens.org"
    exit
fi

# shuffle training data
../../tools/line_shuffle ua.base ua.base.shuffle

# generate feature file
python mkbasicfeature.py ua.base.shuffle ua.base.basicfeature
python mkbasicfeature.py ua.test         ua.test.basicfeature

# make buffer, transform text format to binary format
../../tools/make_feature_buffer ua.base.basicfeature ua.base.buffer
../../tools/make_feature_buffer ua.test.basicfeature ua.test.buffer

# training for 40 rounds
../../svd_feature basicMF.conf num_round=40
# write out prediction from 0040.model
../../svd_feature_infer basicMF.conf pred=40 
