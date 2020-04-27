#!/bin/bash
# For running on CIMS

M2SEXE=../bin/m2s
CONFIGPATH=./config
BENCHMARKPATH=./benchmarktests
BENCHMARKINPUT=./benchmarktests/barnes_input

echo -e "Basic Test Script\n"

declare -a policies=("lru"
                    "flru"
                    "flrup")
for pol in "${policies[@]}"
do
    echo -e "\n$pol Policy"
    $M2SEXE --x86-sim detailed \
     --x86-config $CONFIGPATH/x86-config.ini \
     --mem-config $CONFIGPATH/mem-config-${pol}.ini \
     ./test-threads 4
done
