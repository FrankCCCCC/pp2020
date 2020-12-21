# !/bin/bash

COLOR_REST='\e[0m'
COLOR_GREEN='\e[0;32m'
COLOR_BLUE='\e[0;34m'
COLOR_RED='\e[0;31m'

echo -e "Compiling..."
# nvcc  -O3 -std=c++11 -Xptxas=-v -arch=sm_61  -lm hw4-1_corr.cu -o ./execs/hw4-1_corr
nvcc  -O3 -std=c++11 -Xptxas=-v -arch=sm_61  -lm hw4-1.cu -o ./execs/hw4-1

# rm prof/prof.nvvp
# srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics gld_throughput -o prof/prof.nvvp ./execs/hw4-1_corr /home/pp20/share/hw4-1/cases/c01.1 ./out/c01.1.out
# srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics shared_efficiency gld_throughput -o prof/prof16-16.nvvp ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out
# srun -p prof -N1 -n1 --gres=gpu:1 nvprof -f --metrics shared_efficiency ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out
# srun -p prof -N1 -n1 --gres=gpu:1 nvprof -f --metrics l2_l1_write_throughput,2_l1_write_throughputs,l2_l1_read_throughput,gld_throughput ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out

rm -f prof/gld_out.txt

metrics=(gld_throughput l2_l1_write_throughput l2_l1_write_throughputs l2_l1_read_throughput)
metrics_num=${#metrics[@]}
Bs=(8 16 32 64)
Bs_num=${#Bs[@]}
dim_x=8
dim_y=8

echo -e "${COLOR_GREEN}->${metrics[0]}${COLOR_REST}"
for ((idx=0;idx<${Bs_num};idx++))
do
    met=${metrics[0]}
    echo -e "${COLOR_BLUE}Metric: ${met} Param: ${Bs[idx]} ${dim_x} ${dim_y}${COLOR_REST}"
    echo -e "srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}"
    srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}
done

# echo -e "${COLOR_GREEN}->${metrics[1]}${COLOR_REST}"
# for ((idx=0;idx<${Bs_num};idx++))
# do
#     met=${metrics[1]}
#     echo -e "${COLOR_BLUE}Metric: ${met} Param: ${Bs[idx]} ${dim_x} ${dim_y}${COLOR_REST}"
#     echo -e "srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}"
#     srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}
# done

echo -e "${COLOR_GREEN}->${metrics[2]}${COLOR_REST}"
for ((idx=0;idx<${Bs_num};idx++))
do
    met=${metrics[2]}
    echo -e "${COLOR_BLUE}Metric: ${met} Param: ${Bs[idx]} ${dim_x} ${dim_y}${COLOR_REST}"
    echo -e "srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}"
    srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}
done

echo -e "${COLOR_GREEN}->${metrics[3]}${COLOR_REST}"
for ((idx=0;idx<${Bs_num};idx++))
do
    met=${metrics[3]}
    echo -e "${COLOR_BLUE}Metric: ${met} Param: ${Bs[idx]} ${dim_x} ${dim_y}${COLOR_REST}"
    echo -e "srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}"
    srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics ${met} ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out ${Bs[idx]} ${dim_x} ${dim_y}
done