#!/bin/bash
# For running on CIMS

M2SEXE=../bin/m2s
CONFIGPATH=./config
BENCHMARKEXE=./benchmarktests/fft.i386
BENCHMARKARGS="-p4 -m16"
OUTPATH=./out/
OUTFILE=fft

echo -e "FFT Benchmark ( $BENCHMARKARGS )\n" | tee -a ${OUTPATH}${OUTFILE}.txt 

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
