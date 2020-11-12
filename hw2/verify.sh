# !/bin/bash

dir="verify_temp"
exes="execs"
outs="out"

echo -e "Removing Old Files"
rm ./execs/hw2a ./sample/hw2seq
rm ./out/tc1_hw2a.png ./out/tc1_hw2seq.png ./out/tc2_hw2a.png ./out/tc2_hw2seq.png ./out/tc3_hw2seq.png ./out/tc3_hw2a.png

echo -e "Compiling"
bash ./compile.sh
cd sample
make
cd ../

echo -e "Testcase1 1000 -2 2 -2 2 800 800"
./sample/hw2seq ./out/tc1_hw2seq.png 1000 -2 2 -2 2 800 800
./execs/hw2a ./out/tc1_hw2a.png 1000 -2 2 -2 2 800 800
hw2-diff ./out/tc1_hw2seq.png ./out/tc1_hw2a.png

echo -e "Testcase2 3000 -2 2 -2 2 1600 1600"
./sample/hw2seq ./out/tc2_hw2seq.png 3000 -2 2 -2 2 1600 1600
./execs/hw2a ./out/tc2_hw2a.png 3000 -2 2 -2 2 1600 1600
hw2-diff ./out/tc2_hw2seq.png ./out/tc2_hw2a.png

echo -e "Testcase3 5000 -1 1 -1 1 1600 1600"
./sample/hw2seq ./out/tc3_hw2seq.png 5000 -1 1 -1 1 1600 1600
./execs/hw2a ./out/tc3_hw2a.png 5000 -1 1 -1 1 1600 1600
hw2-diff ./out/tc3_hw2seq.png ./out/tc3_hw2a.png