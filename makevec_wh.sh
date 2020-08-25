#!/bin/bash

if [ $# -lt 1 ]
then
        echo "Usage: $0 <vec file>"
        exit
fi

VECFILE=$1

NLINES=`wc -l $VECFILE| awk '{print $1}'`
DIM=`head -n1 $VECFILE | awk '{print NF}'`

#with header vec
echo "$NLINES $DIM" > $VECFILE.wh.vec
cat $VECFILE >> $VECFILE.wh.vec

