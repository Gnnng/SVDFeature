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

# shuffle the training data and group it by user
../../tools/svdpp_randorder ua.base ua.base.order
../../tools/line_reorder ua.base ua.base.order ua.base.group

# generate  basic feature file for user grouped training  and test data
python mkbasicfeature.py ua.base.group ua.base.group.basicfeature
python mkbasicfeature.py ua.test ua.test.basicfeature

#generate implicit feedback feature for user grouped training and test data
python mkimplicitfeedbackfeature.py ua.base ua.base.group ua.base.feedbackfeature
python mkimplicitfeedbackfeature.py ua.base ua.test ua.test.feedbackfeature

#make buffer for training and test feature file
../../tools/make_ugroup_buffer ua.base.group.basicfeature buffer.base.svdpp -fd ua.base.feedbackfeature
../../tools/make_ugroup_buffer ua.test.basicfeature buffer.test.svdpp -fd ua.test.feedbackfeature 

# training for 40 rounds
../../svd_feature implicitFeedback.conf num_round=40 

# write out prediction from 0040.model
../../svd_feature_infer implicitFeedback.conf pred=40 

