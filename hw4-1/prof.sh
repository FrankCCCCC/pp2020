# !/bin/bash
echo -e "Compiling..."
# nvcc  -O3 -std=c++11 -Xptxas=-v -arch=sm_61  -lm hw4-1_corr.cu -o ./execs/hw4-1_corr
nvcc  -O3 -std=c++11 -Xptxas=-v -arch=sm_61  -lm hw4-1.cu -o ./execs/hw4-1

# rm prof/prof.nvvp
# srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics gld_throughput -o prof/prof.nvvp ./execs/hw4-1_corr /home/pp20/share/hw4-1/cases/c01.1 ./out/c01.1.out
srun -p prof -N1 -n1 --gres=gpu:1 nvprof --metrics shared_efficiency gld_throughput -o prof/prof16-16.nvvp ./execs/hw4-1 /home/pp20/share/hw4-1/cases/c21.1 ./out/c21.1.out