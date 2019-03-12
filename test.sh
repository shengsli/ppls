#!/bin/bash
test_num="$1"
item_num="$2"
thread_num="$3"
succ_num=0
for i in `seq 1 $test_num`; do
    gcc -o ex2 ex2.c -DNITEMS=$item_num -DNTHREADS=$thread_num -DSHOWDATA=1 -lpthread
    count=`./ex2 | grep -c "Well done"`
	((succ_num++))
	if [[ $count = 0 ]]; then
		echo "test $count failed."
		exit -1;
	else
		echo "test $i succeeded."
	fi
done

if [ "$succ_num" == "$test_num" ]; then
	echo "well done. all $test_num tests succeeded."
fi
