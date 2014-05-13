#!/usr/bin/python
#normal positive negative sample
import sys
import bisect
import random

    
def sample_neg( fo, user_datal, user_data,  pos_bound, nsample, num_item ):
    for i, r in user_datal:
        #the items with rate >= pos_bound as positive sample
        if r >= pos_bound:
            # output pos sample
            fo.write( '%d\t%d\t%d\n' % ( last_uid, i, r ) )
            sampled = {}
            #sample nsample fold negative sample 
            for itr in xrange( nsample ):
                while True:                    
                    idx = random.randint(0, num_item-1)
                    if (not user_data.has_key( idx )) and  (not sampled.has_key(idx)):
                        break;
                fo.write( '%d\t%d\t0\n' % ( last_uid, idx ) )
                sampled[ idx ] = 1

if __name__ == '__main__':    
    if len(sys.argv) < 4:
        print 'Usage: input output  nsample [pos_bound]'
        exit(0)
    
    if len(sys.argv) > 4:
        pos_bound = int( sys.argv[4] )
    else:
        pos_bound = 4 
    # seed data for repeatable experiment
    random.seed( 0 )
    # num of total items     
    num_item  = 1682
    # negtive sample size 
    nsample = int( sys.argv[3] )

    # start generation
    last_uid  = -1
    user_data = {}
    user_datal= []
    fi = open( sys.argv[1], 'r' )
    fo = open( sys.argv[2], 'w' )       
    for line in fi:        
        arr = line.split()
        #user id starts from 0
        uid = int( arr[0] )-1
        #item id starts from 0
        iid = int( arr[1] )-1
        rate= int( arr[2] )
        # block end, start generation
        if uid != last_uid:
            if last_uid != -1:
                sample_neg( fo, user_datal, user_data,  pos_bound, nsample, num_item )
            user_data = {}
            user_datal= []
            last_uid = uid
            
        user_data[ iid ] = rate
        user_datal.append( ( iid,rate ) )
    
    sample_neg( fo, user_datal, user_data,  pos_bound, nsample, num_item )
    fi.close()
    fo.close()
