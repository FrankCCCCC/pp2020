# !/bin/bash

echo -e "Setting up Environment..."
rm ./execs/hw3
rm ./out/c01.1.out

echo -e "Compiling..."
bash compile.sh
./execs/hw3 ./sample/cases/c01.1 ./out/c01.1.out

echo -e "Diff Result:"
diff sample/cases/c01.1.out out/c01.1.out