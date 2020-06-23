#!/bin/bash
master_method="f m"
slave_method="f m"
Case="small big empty"
for mm in $master_method; 
do
    for sm in $slave_method;
    do
        for c in $Case;
        do
	    ./user_program/master 1 ./testcase/$c.txt $mm&
            ./user_program/slave 1 ./result/$c.txt $sm "127.0.0.1"
            result=$(diff -y -W 72 ./testcase/$c.txt ./result/$c.txt)
            if [ $? -eq 0 ]
            then
                echo "Master Method: $mm Slave Method: $sm Case: $c Passed\n"
            else
                echo "Master Method: $mm Slave Method: $sm Case: $c Failed"
                echo "$result\n"
            fi
        done
    done
done
