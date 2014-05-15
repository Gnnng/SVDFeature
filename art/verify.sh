#!/bin/bash
rm run.log

# mkdir test_round
#for i in  0 1 2 3 4 5 6 7 8 9 ; do
	#for j in 0 1 2 3 4 5 6 7 8 9; do
		for k in 0 1 2 3 4 5 6 7 8 9; do
			for l in 0 1 2 3 4 5 6 7 8 9; do
				./run.sh 100 8 0.021${k}${l} 0.02401>> run.log
				./verify.exe
				# mv i_bias.txt ./test_round/i_bias.txt.$i
			done
		done	
	#done
#done
