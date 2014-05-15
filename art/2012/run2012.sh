#!/bin/bash
# example running script for art

# default value
round_val=100
factor_val=10
lambda=0.050
learning=0.020
# arguments from shell
argv1=$1
argv2=$2
argv3=$3
argv4=$4

echo $argv1

if [ ${#argv1} -gt 0 ]; then round_val=$argv1; fi
if [ ${#argv1} -gt 0 ]; then factor_val=$argv2; fi
if [ ${#argv3} -gt 0 ]; then lambda=$argv3; fi
if [ ${#argv4} -gt 0 ]; then learning=$argv4; fi

echo Parameter list
echo round is $round_val
echo num_factor is $factor_val
echo lambda is $lambda
echo learning_rate is $learning

# make buffer, transform text format to binary format
../../tools/make_feature_buffer art.base art.base.buffer
../../tools/make_feature_buffer art.base art.test.buffer
if [ ! -d modeldata ]; then mkdir modeldata; fi
# training for $round_val rounds
../../svd_feature art.conf num_round=$round_val num_factor=$factor_val wd_user=$lambda wd_item=$lambda learning_rate=$learning
# write out prediction from model
../../svd_feature_infer art.conf pred=$round_val num_facotr=$factor_val wd_user=$lambda wd_item=$lambda learning_rate=$learning
