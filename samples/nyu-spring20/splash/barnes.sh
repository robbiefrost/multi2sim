#!/bin/bash
# For running on CIMS
echo -e "Barnes Benchmark\n"
m2sExe=$1
barnesPath=$2
configPath=$3
declare -a policies=("lru"
                    "flru"
                    "flrup")
for pol in "${policies[@]}"
do
    echo -e "\n$pol Policy"
    $m2sPath --x86-sim detailed \
     --x86-config $configPath/x86-config.ini \
     --mem-config $configPath/mem-config-$pol.ini \
     $barnesPath/barnes.i386 < $barnesPath/data-test/input
done