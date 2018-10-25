#!/bin/bash
  
if [ $# -lt 2 ]
then
        echo "Usage: $0 <vec dimension> <window length>"
        exit
fi

DATA_DIR=$PWD/data_node2vec/
DIM=$1
WINDOW=$2
DATA_FILE=$DATA_DIR/dbpedia.subset.txt.100000
WVEC_FILE=$DATA_DIR/dbpedia.sgns.${DIM}
CTXT_WVEC_FILE=$DATA_DIR/dbpedia.cwvec.$DIM

#To generalize, write a script to generate the graph
GRAPH_FILE=$DATA_DIR/mat.txt.s.20k

if [ ! -e $WVEC_FILE ]
then
	echo "Training word vectors on DBPedia"
	./word2vec -train $DATA_FILE -size $DIM -cbow 0 -output $WVEC_FILE -iter 3 -ns 5 -window 10   
fi

./node2vec -train ${GRAPH_FILE} -output $CTXT_WVEC_FILE -pt ${WVEC_FILE}.bin -size $DIM -trace 3 -window $WINDOW
