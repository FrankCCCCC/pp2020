# !bin/bash

echo -e "Start Experiments"

echo -e "Compiling hw1"
make
echo -e "hw1 Compiled"

exp_testcase=35
rep_f_name="report"
rep_f_format=".csv"
exp_n=5368698
exp_nodes=1
node=1
exp_procs=(1 2 4)
for proc in "${exp_procs[@]}"
do  
    echo -e "Experiment: $proc procs"
    srun -n$proc -N$node ./hw1 exp_n /home/pp20/share/hw1/testcases/$exp_testcase.in ./out/$exp_testcase.out measure/$rep_f_name$proc-$node$rep_f_format >> exp.txt
    echo -e "" >> exp.csv
done


