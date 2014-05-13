#
#   This file defines the implicit feedback  feature generator for movielen feature
#
import sys
#make implicit feedback features
#the input file1 format:userid \t itemid \t rate \n
#the input file2 format:userid \t itemid \t rate \n, which is grouped by user
#the output format:rate \t number of user group \t number of user implicit feedback \t fid1:fvalue1, fid2:fvalue2 ... \n

#user implicit feedback
def userfeedback(fname):
    fi = open(fname,'r')
    feedback = {}
    for line in fi:
	attr = line.strip().split('\t')
	uid = int(attr[0])-1
	iid = int(attr[1])-1
	if feedback.has_key(uid):
	    feedback[uid].append(iid)
	else:
	    feedback[uid] = [iid]
    fi.close()
    return feedback

#group num and order of the grouped training data
def usergroup(fname):
    fi = open(fname,'r')
    userorder = []
    groupnum = {}
    lastuid = -1
    for line in fi:
        attr = line.strip().split('\t')
	uid = int(attr[0])-1
	if groupnum.has_key(uid):
	    groupnum[uid] += 1
	else:
	    groupnum[uid] = 1
	if uid != lastuid:
	    userorder.append(uid)
	lastuid = uid
    fi.close()
    return userorder,groupnum

#make implict feedback feature, one line for a user, wihch is in the order of the grouped training data 
#the output format:rate \t number of user group \t number of user implicit feedback \t fid1:fvalue1, fid2:fvalue2 ... \n
def mkfeature(fout,userorder,groupnum,feedback):
    fo = open(fout,'w')
    for uid in userorder:
	gnum = groupnum[uid]
	fnum = len(feedback[uid])
	fo.write('%d\t%d\t' %(gnum,fnum))
	for i in feedback[uid]:
	    fo.write('%d:%.6f ' %(i,pow(fnum,-0.5)))
	fo.write('\n')

if __name__ == '__main__':
    if len(sys.argv) < 4:
	print 'usage:<training_file> <grouped training_file> <output>'
	exit(-1)
    ftrain = sys.argv[1]
    fgtrain = sys.argv[2]
    fout = sys.argv[3]
    feedback = userfeedback(ftrain)
    userorder,groupnum = usergroup(fgtrain)
    #make features and print them  out in file fout 
    mkfeature(fout,userorder,groupnum,feedback)
