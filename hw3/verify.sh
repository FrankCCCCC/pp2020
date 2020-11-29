# !/bin/bash

echo -e "Setting up Environment..."
rm ./execs/hw3

echo -e "Compiling..."
bash compile.sh

# echo -e "Testcase 1"
# ./execs/hw3 ./sample/cases/c01.1 ./out/c01.1.out
# echo -e "Diff Result:"
# diff sample/cases/c01.1.out out/c01.1.out

# tcs=(0 1 2)
# tcs=(0 1 2 3 4 5 6 7 8 9 10 15)
tcs=(0 17 18)
# tcs=(0 20)
tc_num=${#tcs[@]}
# vatc_numr=$((tc_num+1))

for ((idx=1;idx<${tc_num};idx++))
do
    echo -e "${idx}-Testcase ${tcs[idx]}"

    num=0
    if [ "${tcs[idx]}" -gt 9 ]; then
        num=${tcs[idx]}
    else
        num="0${tcs[idx]}"
    fi
    rm out/c${num}.1.out

    ./execs/hw3 ./sample/cases/c${num}.1 ./out/c${num}.1.out
    echo -e "Diff Result:"
    diff sample/cases/c${num}.1.out out/c${num}.1.out
    # diff sample/cases/c0${tcs[idx]}.1 out/c0${tcs[idx]}.1.out
    echo -e "----------------------------------------"
    echo -e ""
done