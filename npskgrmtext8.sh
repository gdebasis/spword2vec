#!/bin/bash

if [ $# -lt 8 ]
then
        echo "Usage: $0 <min-size> <max-size> <events (error/iter/rdim/cf)> (bbbb) (b:=0/1) <wsize> <ns>"
        exit
fi

MINSIZE=$1
SIZE=$2
ERROR_EVENT=$3
ITER=$4
DIM=$5
CF=$6
WSIZE=$7
NS=$8
ALL=$1.$2.$3.$4.$5.$6.$7.$8

OUTFILE=wvecs/swvecs.$ALL

REF_DATA_WSIM353=datasets/wordsim353/combined.norm.tsv
REF_DATA_MEN=datasets/men/MEN_dataset_natural_form_full

if [ ! -e $OUTFILE.bin ]
then
	./word2svec -train text8/text8 -output $OUTFILE -minsize $MINSIZE -cbow 0 -dropout 1 -size $SIZE -window $WSIZE -negative $NS -hs 0 -sample 1e-4 -threads 4 -iter 3 -error-event $ERROR_EVENT -iter-event $ITER -rdim-event $DIM -cf-event $CF
	echo
fi

echo "Evaluating against the Wsim353 dataset"
./distwordpairs $OUTFILE.bin $REF_DATA_WSIM353 > tmp
rho_1=`tail -n1 tmp | awk -F '=' '{print $2}'`

echo "Evaluating against the MEN dataset"
./distwordpairs $OUTFILE.bin $REF_DATA_MEN > tmp  
rho_2=`tail -n1 tmp | awk -F '=' '{print $2}'`

./stat-npwvec.sh $OUTFILE.vec | tail -n1 > stats
ceildim=`cat stats | awk '{print $NF}'`

cratio=`cat stats | awk '{print $3}'`
avgdim=`cat stats | awk '{print $6}'`

CEILDIMOUTFILE=wvecs/wvecs.$ceildim
if [ ! -e $CEILDIMOUTFILE.bin ]
then
	./word2svec -train text8/text8 -output $CEILDIMOUTFILE -cbow 0 -size $ceildim -window $WSIZE -negative $NS -hs 0 -sample 1e-4 -threads 4 -iter 3
	echo
fi

./distwordpairs $CEILDIMOUTFILE.bin $REF_DATA_WSIM353 > tmp
ceil_rho_1=`tail -n1 tmp | awk -F '=' '{print $2}' | sed 's/ //'`

./distwordpairs $CEILDIMOUTFILE.bin $REF_DATA_MEN > tmp  
ceil_rho_2=`tail -n1 tmp | awk -F '=' '{print $2}' | sed 's/ //'`

MAXDIMOUTFILE=wvecs/wvecs.$SIZE.$WSIZE.$NS
if [ ! -e $MAXDIMOUTFILE.bin ]
then
	./word2svec -train text8/text8 -output $MAXDIMOUTFILE -cbow 0 -size $SIZE -window $WSIZE -negative $NS -hs 0 -sample 1e-4 -threads 4 -iter 3
	echo
fi

./distwordpairs $MAXDIMOUTFILE.bin $REF_DATA_WSIM353 > tmp
max_rho_1=`tail -n1 tmp | awk -F '=' '{print $2}' | sed 's/ //'`

./distwordpairs $MAXDIMOUTFILE.bin $REF_DATA_MEN > tmp  
max_rho_2=`tail -n1 tmp | awk -F '=' '{print $2}' | sed 's/ //'`

#echo "$MINSIZE $SIZE $ITER $DIM $CF $cratio $avgdim $rho_1 $ceil_rho_1 $max_rho_1 $rho_2 $ceil_rho_2 $max_rho_2" > res
echo "$MINSIZE $SIZE $WSIZE $NS $cratio $avgdim $rho_1 $ceil_rho_1 $max_rho_1 $rho_2 $ceil_rho_2 $max_rho_2" > res

rm tmp
rm stats

cat res

