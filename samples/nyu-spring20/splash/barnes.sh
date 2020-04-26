#!/bin/bash
# For running on CIMS
# Make sure you've edited data-test/input for 4 cores
# run with "2>&1 | tee log.txt" at the end to output to terminal and log.txt
# start within tmux so you can detach from the session and leave it running
echo -e "Barnes Benchmark"
m2sExe=$1
barnesPath=$2
configPath=$3
declare -a policies=("lru"
                    "flru"
                    "flrup")
for pol in "${policies[@]}"
do
    echo -e "\n$pol Policy"
    $m2sExe --x86-sim detailed \
     --x86-config x86-config.ini \
     --mem-config mem-config-$pol.ini \
     $barnesPath/barnes.i386 < $barnesPath/data-test/input
done