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

# generate negative samples for training data
python sampleneg.py ua.base.group ua.base.group.3N 3 4

# generate  basic feature file for user grouped training data
python mkbasicfeature.py ua.base.group.3N ua.base.group.3N.basicfeature

# generate new format of test feature which directly outputs ranking
python mktestrank.py ua.base.group ua.test ua.test.basicfeature

#generate implicit feedback feature for user grouped training and test data
python mkimplicitfeedbackfeature.py ua.base ua.base.group.3N ua.base.group.3N.feedbackfeature
python mkimplicitfeedbackfeaturetest.py ua.test.basicfeature ua.test.feedbackfeature

#make buffer for training and test feature file
../../tools/make_ugroup_buffer ua.base.group.3N.basicfeature buffer.base.svdpp -fd ua.base.group.3N.feedbackfeature -scale_score 5 
../../tools/make_ugroup_buffer ua.test.basicfeature buffer.test.svdpp -fd ua.test.feedbackfeature -scale_score 1 -max_block 400

# training for 40 rounds
../../svd_feature pairwiseRankML100K.conf num_round=40 

# write out prediction from 0040.model
../../svd_feature_infer pairwiseRankML100K.conf pred=40 

# evaluate results using the metric of Precision@20
python eval.py pred.txt
