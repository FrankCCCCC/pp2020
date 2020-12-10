# !/bin/bash

# rm ./out/candy.png
# rm ./out/jerry.png
# rm ./out/large-candy.png
# rm ./execs/hw4-1

# bash compile.sh

# srun -n 1 --gres=gpu:1 ./execs/lab5 /home/pp20/share/lab4/testcases/candy.png ./out/candy.png
# png-diff ./out/candy.png /home/pp20/share/lab4/testcases/candy.out.png

# srun -n 1 --gres=gpu:1 ./execs/lab5 /home/pp20/share/lab4/testcases/jerry.png ./out/jerry.png
# png-diff ./out/jerry.png /home/pp20/share/lab4/testcases/jerry.out.png

# srun -n 1 --gres=gpu:1 ./execs/lab5 /home/pp20/share/lab4/testcases/large-candy.png ./out/large-candy.png
# png-diff ./out/large-candy.png /home/pp20/share/lab4/testcases/large-candy.out.png

program="hw4-1"

echo -e "Setting up Environment..."
rm ./execs/${program}

echo -e "Compiling..."
bash compile.sh

# tcs=(0 1 2)
# tcs=(0 1 2 3 4 5 6 7 15 17)
# tcs=(0 17 18)
tcs=(0 1 2 3 15)
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

    echo -e "./execs/${program} ./sample/cases/c${num}.1 ./out/c${num}.1.out"
    
    ./execs/${program} ./sample/cases/c${num}.1 ./out/c${num}.1.out
    echo -e "Diff Result:"
    diff sample/cases/c${num}.1.out out/c${num}.1.out
    # diff sample/cases/c0${tcs[idx]}.1 out/c0${tcs[idx]}.1.out
    echo -e "----------------------------------------"
    echo -e ""
done