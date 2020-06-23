master_method="f m"
slave_method="f m"
Case="small big empty"
for mm in $master_method; 
do
    for sm in $slave_method; 
    do
        for c1 in $Case;
        do
            for c2 in $Case; 
            do
                ./user_program/master 2 ./testcase/$c1.txt ./testcase/$c2.txt $mm&
    	        ./user_program/slave 2 ./result/$c1.txt ./result/$c2.txt $sm 127.0.0.1
	        result=$(diff -y -W 72 ./testcase/$c1.txt ./result/$c1.txt)
                if [ $? -eq 0 ]
	            then
			:
                    else
                        printf "Master Method: $mm Slave Method: $sm Case: $c1 $c2 : $c1 Failed\n"
                        printf "$result\n"	
	        fi
                result=$(diff -y -W 72 ./testcase/$c2.txt ./result/$c2.txt)
                if [ $? -eq 0 ]
	            then
                        printf "Master Method: $mm Slave Method: $sm Case: $c1 $c2 Passed\n\n"
	            else
                        printf "Master Method: $mm Slave Method: $sm Case: $c1 $c2 : $c2 Failed\n"
                        printf "$result\n\n"	
	        fi
done
    done
        done
            done
