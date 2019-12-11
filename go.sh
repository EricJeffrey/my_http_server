#!/bin/bash
# printf 'build main.cpp? (Y/n)'
# read a
# if [[ $a == 'y' || $a == 'Y' || $a == '' ]]; then
#     echo 'building main.cpp, output dir: ./build'
#     g++ -Wall --static -o build/main.out main.cpp -lpthread
#     # g++ -Wall -g -rdynamic -o build/main.out main.cpp -lpthread
# fi
build_dir=./build
if [ -d "$build_dir" ]; then
    echo $build_dir exists
else
    echo 'making dir $build_dir'
    mkdir build/
fi
echo 'building main.cpp'
g++ -Wall -o build/main.out main.cpp main_app.cpp -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
printf 'build complete, chdir to ./build, execute outputfile? (Y or N)'
cd build/
read a
if [[ $a == 'y' || $a == 'Y' || $a == '' ]]
then
    echo '-----------------------------------'
    ./main.out
else
    echo 'exiting, bye'
fi