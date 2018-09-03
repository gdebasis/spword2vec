#!/bin/bash

if [ $# -lt 4 ]
then
        echo "Usage: $0 <min-size> <max-size (>min-size)> <context-size> <negative sampling size>>"
        exit
fi

MINSIZE=$1
SIZE=$2
CONTEXT=$3
NS=$4

./npskgrmtext8.sh $MINSIZE $SIZE 0 0 0 $CONTEXT $NS
cat res
rm res 
