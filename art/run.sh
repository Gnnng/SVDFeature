#!/bin/bash
# example running script for art

# make buffer, transform text format to binary format
../tools/make_feature_buffer art.base art.base.buffer
../tools/make_feature_buffer art.base art.test.buffer

# default value
round_val=60
factor_val=3
lambda=0.004

# arguments from shell
argv1=$1
argv2=$2
argv3=$3

if [ ${#argv1} -gt 0 ]; then
	round_val=$argv1
fi

if [ ${#argv1} -gt 0 ]; then
	factor_val=$argv2
fi

if [ ${#argv3} -gt 0 ]; then
	lambda=$argv3
fi

# training for $round_val rounds
../svd_feature art.conf num_round=$round_val num_factor=$factor_val wd_user=$lambda wd_item=$lambda
# write out prediction from model
../svd_feature_infer art.conf pred=$round_val num_facotr=$factor_val wd_user=$lambda wd_item=$lambda
