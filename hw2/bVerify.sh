# !/bin/bash

seq_dir="./sample"
seq="hw2seq"
execs="./execs"
# target_dir="./execs"
target="hw2a"
out_dir="./out/hw2a"
out="_tc"
tc_num=5

mkdir ${out_dir}

echo -e "Removing Old Files"
rm ${execs}/${target} ${execs}/${seq}
for ((idx=1;idx<=${tc_num};idx++))
do
    rm ${out_dir}/${target}${out}${idx}.png ${out_dir}/${seq}${out}${idx}.png
    # echo "${idx}"
done
# rm ${out_dir}/tc1_hw2a.png ${out_dir}/tc1_hw2seq.png ./out/tc2_hw2a.png ./out/tc2_hw2seq.png ./out/tc3_hw2seq.png ./out/tc3_hw2a.png ./out/tc4_hw2seq.png ./out/tc4_hw2a.png

echo -e "Compiling"
bash ./compile.sh
cd ${eq_dir}
make
cd ../

params=("1000 -2 2 -2 2 800 800",)

for ((idx=1;idx<=${tc_num};idx++))
do
    echo -e "Testcase${idx} ${params[idx]}"
    ${execs}/${seq} ${out_dir}/${seq}${out}${idx}.png ${params[idx]}
    ${execs}/${target} ${out_dir}/${target}${out}${idx}.png ${params[idx]}
    hw2-diff ${out_dir}/${seq}${out}${idx}.png ${out_dir}/${target}${out}${idx}.png
    echo "${idx}"
done

# echo -e "Testcase1 1000 -2 2 -2 2 800 800"
# ./sample/hw2seq ./out/tc1_hw2seq.png 1000 -2 2 -2 2 800 800
# ./execs/hw2a ./out/tc1_hw2a.png 1000 -2 2 -2 2 800 800
# hw2-diff ./out/tc1_hw2seq.png ./out/tc1_hw2a.png

# echo -e "Testcase2 3000 -2 2 -2 2 1600 1600"
# ./sample/hw2seq ./out/tc2_hw2seq.png 3000 -2 2 -2 2 1600 1600
# ./execs/hw2a ./out/tc2_hw2a.png 3000 -2 2 -2 2 1600 1600
# hw2-diff ./out/tc2_hw2seq.png ./out/tc2_hw2a.png

# echo -e "Testcase3 1000 -1 2 -2 1 1597 1597"
# ./sample/hw2seq ./out/tc3_hw2seq.png 1000 -1 2 -2 1 1597 1597
# ./execs/hw2a ./out/tc3_hw2a.png 1000 -1 2 -2 1 1597 1597
# hw2-diff ./out/tc3_hw2seq.png ./out/tc3_hw2a.png

# echo -e "Testcase4 10000 -5 6 -7 8 56 78"
# ./sample/hw2seq ./out/tc4_hw2seq.png 10000 -5 6 -7 8 56 78
# ./execs/hw2a ./out/tc4_hw2a.png 10000 -5 6 -7 8 56 78
# hw2-diff ./out/tc4_hw2seq.png ./out/tc4_hw2a.png

# echo -e "Testcase5 5000 -1 1 -1 1 1600 1600"
# ./sample/hw2seq ./out/tc5_hw2seq.png 5000 -1 1 -1 1 1600 1600
# ./execs/hw2a ./out/tc5_hw2a.png 5000 -1 1 -1 1 1600 1600
# hw2-diff ./out/tc5_hw2seq.png ./out/tc5_hw2a.png