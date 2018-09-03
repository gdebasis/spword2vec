#!/bin/bash

if [ $# -lt 1 ]
then
        echo "Usage: $0 <max-size (>100)>"
        exit
fi

SIZE=$1

rm allres

for CONTEXT in 5 10 20
do
	for NS in 5 10 20
	do
		for minsize in 5 10 20 50 100 
		do
			./npskgrmtext8.sh $minsize $SIZE 0 0 0 $CONTEXT $NS
			cat res >> allres
			rm res 
		done
	done
done 
