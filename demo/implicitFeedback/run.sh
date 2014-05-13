#make buffer for training and test feature file
../../tools/make_ugroup_buffer ua.base.group.example buffer.base.svdpp -fd ua.base.feedbackexample
../../tools/make_ugroup_buffer ua.test.example buffer.test.svdpp -fd ua.test.feedbackexample

# training for 40 rounds
../../svd_feature implicitFeedback.conf num_round=40

# write out prediction from 0040.model
../../svd_feature_infer implicitFeedback.conf pred=40

