# !/bin/bash

g++ ./sample/lab3_pthread.cc -o ./sample/lab3_pthread -pthread -lm
g++ lab3_pthread.cc -o lab3_pthread -pthread -lm

tc_n=5
r=(10000, 1067212, 9183439, 1781232, 212125892, 1401149118)
k=(199, 555, 823, 101, 101, 14011491183)

# tc_n = $(( tc_n - 1))

for i in `seq 0 ${tc_n}`
do
    echo -e "Testcase ${i}: r=${r[i]} k=${k[i]}"

    res_std=`srun -c8 -n1 ./sample/lab3_pthread ${r[i]} ${k[i]}`
    res_self=`srun -c8 -n1 ./lab3_pthread ${r[i]} ${k[i]}`
    res_self_time=`time srun -c8 -n1 ./lab3_pthread ${r[i]} ${k[i]}`
    # res_self=res_self+1

    if [ $res_std -eq $res_self ]
    then
        echo -e "Testcase ${i} Passed, Result: ${res_self}, Correct: ${res_std}, Time: ${res_self_time}"
    else
        echo -e "Testcase ${i} Failed, Result: ${res_self}, Correct: ${res_std}, Time: ${res_self_time}"
    fi
done