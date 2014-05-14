#!/bin/bash
rm run.log
for i in 1 2 3 4 5 6 7 8 9; do
	./run.sh 60 $i >> run.log
	./verify.exe
done

	