#!/usr/bin/python
# make implicit feedback feature for test feature which is for directly outputting ranking   
import sys
import math
if __name__ == '__main__':
    if len(sys.argv) < 3:
        print 'Usage: input output'
        exit(0)
    fi = open(sys.argv[1],'r')
    fo = open(sys.argv[2],'w')
    num = 0
    tlist = []
    for line in fi:
        if line.startswith('2'):
            fo.write('%d %d' %(num,len(tlist)))
            if len(tlist) > 0:
                v = 1.0/math.sqrt(len(tlist))
                for i in tlist:
                    fo.write(' %d:%f' %(i,v))
            fo.write('\n')
            num = 0
            tlist = []
        if line.startswith('-1'):
            iid = int(line.strip().split()[-1].split(':')[0])
            tlist.append(iid)
        num += 1
    fo.write('%d %d' %(num,len(tlist)))
    if len(tlist) > 0:
        v = 1.0/math.sqrt(len(tlist))
        for i in tlist:
            fo.write(' %d:%f' %(i,v))
        fo.write('\n')
    fi.close()
    fo.close()
        
