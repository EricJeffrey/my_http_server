#!/bin/bash
echo 'building main.cpp, output dir: ./build'
g++ -Wall -g -rdynamic -o build/main.out main.cpp -lpthread
echo 'build complete, chdir to ./build, execute outputfile? (Y or N)'
cd build/
read to_execute
if [[ $to_execute == 'y' || $to_execute == 'Y' ]]
then
    ./main.out
else
    echo 'exiting, bye'
fi