# !/bin/bash
bash ./compile.sh
cd sample
make
cd ../
./sample/hw2seq ./out/hw2seq.png 3000 -2 2 -2 2 1600 1600
srun ./execs/hw2a ./out/hw2a.png 3000 -2 2 -2 2 1600 1600
hw2-diff ./out/hw2seq.png ./out/hw2a.png