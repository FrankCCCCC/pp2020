# !/bin/sh
COLOR_REST='\e[0m'
COLOR_GREEN='\e[0;32m'
COLOR_BLUE='\e[0;34m'
COLOR_RED='\e[0;31m'
program="hw4-1_t"

echo -e "${COLOR_BLUE}Setting up Environment...${COLOR_REST}"
rm ./execs/${program}

echo -e "${COLOR_BLUE}Compiling...${COLOR_REST}"
cd timer
make
mv ${program} ../execs/${program}
cd ..

# runner='srun -p prof -N1 -n1 --gres=gpu:1'
runner=''
num='20'
rm out/c${num}.1.out
echo -e "${runner} ./execs/${program} ./sample/cases/c${num}.1 ./out/c${num}.1.out"
${runner} ./execs/${program} ./sample/cases/c${num}.1 ./out/c${num}.1.out