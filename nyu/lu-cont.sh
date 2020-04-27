#!/bin/bash
# For running on CIMS

M2SEXE=../bin/m2s
CONFIGPATH=./config
BENCHMARKEXE=./benchmarktests/lu-cont.i386
BENCHMARKARGS="-p4 -n128"
OUTPATH=./out/
OUTFILE=lu-cont

echo -e "LU-CONT Benchmark ( $BENCHMARKARGS )\n" | tee -a ${OUTPATH}${OUTFILE}.txt

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
