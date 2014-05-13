#!/usr/bin/python
#new format of test feature which is for directly outputting ranking. 

import sys

#get the ground truth items for each user.
def getrelevant(fname):
    glist = {}
    fi = open(fname,'r')
    for line in fi:
        attr = line.strip().split()
        uid = int(attr[0])-1
        iid = int(attr[1])-1
        if glist.has_key(uid):
            glist[uid].append(iid)
        else:
            glist[uid] = [iid]
    fi.close() 
    return glist

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print 'Usage: [train] [groundtruth] [output]'
        exit(0)
    user_num = 943
    item_num = 1682
    fo = open(sys.argv[3],'w')
    for i in xrange(0, item_num):
        fo.write('0\t0 0 1 %d:1\n' %i)
    ft = open(sys.argv[1], 'r')
    #user relevant item
    glist = getrelevant(sys.argv[2])
    last_uid = -1
    ilist = []
    for line in ft:
        attr = line.strip().split()
        uid = int(attr[0])-1
        if uid != last_uid and last_uid != -1:
            fo.write('2\t0 1 0 %d:1\n' %last_uid)
            for item in ilist:
                fo.write('-1\t0 1 0 %d:1\n' %item) 
            for item in glist[last_uid]:
                fo.write('1\t0 1 0 %d:1\n' %item)
            fo.write('4\t0 0 0\n')
            ilist = []
        ilist.append(int(attr[1])-1)
        last_uid = uid
    fo.write('2\t0 1 0 %d:1\n' %last_uid)
    for item in ilist:
        fo.write('-1\t0 1 0 %d:1\n' %item) 
    for item in glist[last_uid]:
        fo.write('1\t0 1 0 %d:1\n' %item)
    fo.write('4\t0 0 0\n')
    ft.close()
    fo.close()

