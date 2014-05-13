#!/usr/bin/python
#eval p@20 
import sys

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'Usage:[pred]'
        exit(0) 
    fi = open(sys.argv[1],'r')
    K = 20
    #precision@k
    P = 0
    unum = 943
    rank = 0
    for line in fi:
        rank = int(line.strip())
        if rank < K:
            P += 1
    P/= float(unum*K)
    print 'Pre@%d:%.4f\n' %(K,P)
    fi.close()
