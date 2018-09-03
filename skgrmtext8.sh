#!/bin/bash

if [ $# -lt 1 ]
then
        echo "Usage: $0 <size>"
        exit
fi

SIZE=$1
ALL=$SIZE

OUTFILE=wvecs/wvecs.$ALL

REF_DATA_WSIM353=datasets/wordsim353/combined.norm.tsv
REF_DATA_MEN=datasets/men/MEN_dataset_natural_form_full

./word2svec -train text8/text8 -output $OUTFILE -cbow 0 -size $SIZE -window 10 -negative 5 -hs 0 -sample 1e-4 -threads 4 -iter 3 

echo "Evaluating against the Wsim353 dataset"
./distwordpairs $OUTFILE.bin $REF_DATA_WSIM353 > tmp
rho_1=`tail -n1 tmp | awk -F '=' '{print $2}'`

echo "Evaluating against the MEN dataset"
./distwordpairs $OUTFILE.bin $REF_DATA_MEN > tmp  
rho_2=`tail -n1 tmp | awk -F '=' '{print $2}'`

echo "$MINSIZE $SIZE $ITER $DIM $CF $rho_1 $rho_2"

rm tmp
 
