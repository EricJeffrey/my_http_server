#!/bin/bash
printf 'build main.cpp? (Y/n)'
read a
if [[ $a == 'y' || $a == 'Y' || $a == '' ]]; then
    echo 'building main.cpp, output dir: ./build'
    g++ -Wall -g -rdynamic -o build/main.out main.cpp -lpthread
fi
printf 'build complete, chdir to ./build, execute outputfile? (Y or N)'
cd build/
read a
if [[ $a == 'y' || $a == 'Y' || $a == '' ]]
then
    ./main.out
else
    echo 'exiting, bye'
fi