#!/bin/bash
# example running script for art

# make buffer, transform text format to binary format
../tools/make_feature_buffer art.base art.base.buffer
../tools/make_feature_buffer art.base art.test.buffer

# training for 60 rounds
../svd_feature art.conf num_round=$1
# write out prediction from *.model
../svd_feature_infer art.conf pred=$1
