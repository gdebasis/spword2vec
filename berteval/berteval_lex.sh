if [ $# -lt 1 ]
then
        echo "Usage: $0 <wordpair file>"
        exit
fi

BIN=..
WORDPAIR=$1
WORDPAIR_NAME=$(basename $WORDPAIR)

if [ ! -e $WORDPAIR.vec ]
then
    #create the vocab
    cat $WORDPAIR | awk '{print $1; print $2}'|sort|uniq > $WORDPAIR_NAME.vocab

    python saveptvecs.py $WORDPAIR_NAME.vocab $WORDPAIR_NAME.vec
fi

$BIN/distwordpairs $WORDPAIR_NAME.vec $WORDPAIR > $WORDPAIR_NAME.res
tail -n1 $WORDPAIR_NAME.res
