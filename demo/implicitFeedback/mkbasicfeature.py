#
#   This file defines the basic svd  feature generator for movielen feature
#
import sys
#make features,including global feature,user feature and item feature
#the input file format:userid \t itemid \t rate \n
#the output format:rate \t number of global features \t number of user features \t number of item features \t gfid:gfvalue ... ufid:ufvalue... ifid:ifvalue...\n
def mkfeature( fin, fout):
    fi = open( fin , 'r' )
    fo = open( fout, 'w' )
    #extract from input file    
    for line in fi:
        arr  =  line.split()               
        uid  =  int(arr[0].strip())
        iid  =  int(arr[1].strip())
        score=  int(arr[2].strip())
        fo.write( '%d\t0\t1\t1\t' %score )
        # Print data,user and item features all start from 0
        fo.write('%d:1 %d:1\n' %(uid-1,iid-1))
    fi.close()
    fo.close()
    print 'generation end'

if __name__ == '__main__':
    if len(sys.argv) < 3:
	print 'usage:<input> <output>'
	exit(-1)
    fin = sys.argv[1]
    fout = sys.argv[2]
    #make features and print them  out in file fout 
    mkfeature( fin, fout)
