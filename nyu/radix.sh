#!/bin/bash
# For running on CIMS

M2SEXE=../bin/m2s
CONFIGPATH=./config
BENCHMARKEXE=./benchmarktests/radix.i386
BENCHMARKARGS="-p4 -n16384"
OUTPATH=./out/
OUTFILE=radix

echo -e "Radix Benchmark ( $BENCHMARKARGS )\n" | tee -a ${OUTPATH}${OUTFILE}.txt 

declare -a policies=("lru"
                    "flru"
                    "flrup")
for pol in "${policies[@]}"
do
    echo -e "\n$pol Policy" | tee -a ${OUTPATH}${OUTFILE}.txt
    $M2SEXE --x86-sim detailed \
     --x86-config $CONFIGPATH/x86-config.ini \
     --mem-config $CONFIGPATH/mem-config-${pol}.ini \
     --mem-report ${OUTPATH}${OUTFILE}_${pol}.txt \
     $BENCHMARKEXE $BENCHMARKARGS | tee -a ${OUTPATH}${OUTFILE}.txt
done
