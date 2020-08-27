if [ $# -lt 2 ]
then
        echo "Usage: $0 <ptvec> <wordpair file>"
        exit
fi

BIN=..
PTVEC=$1
WORDPAIR=$2
WORDPAIR_NAME=$(basename $WORDPAIR)

$BIN/distwordpairs $PTVEC $WORDPAIR > $WORDPAIR_NAME.res
tail -n1 $WORDPAIR_NAME.res

