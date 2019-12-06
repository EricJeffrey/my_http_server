#!/bin/bash
# printf 'build main.cpp? (Y/n)'
# read a
# if [[ $a == 'y' || $a == 'Y' || $a == '' ]]; then
#     echo 'building main.cpp, output dir: ./build'
#     g++ -Wall --static -o build/main.out main.cpp -lpthread
#     # g++ -Wall -g -rdynamic -o build/main.out main.cpp -lpthread
# fi
echo 'building main.cpp, output dir: ./build'
g++ -Wall --static -o build/main.out main.cpp -lpthread
printf 'build complete, chdir to ./build, execute outputfile? (Y or N)'
cd build/
read a
if [[ $a == 'y' || $a == 'Y' || $a == '' ]]
then
    ./main.out
else
    echo 'exiting, bye'
fi