if [ $# -lt 2 ]
then
        echo "usage: $0 <analogy dataset> <case normalize (y/n)>"
        exit
fi

ANALOGY=$1
NORMALIZE=$2

BIN=..
ANALOGY_NAME=$(basename $ANALOGY).$NORMALIZE
ANALOGY_VEC=$ANALOGY_NAME.cvec

#A temp analogy data file (either a copy of original or normalized)
cat $ANALOGY| awk -v n=$NORMALIZE '{for (i=1;i<=NF;i++) { if (i==NF) c="\n"; else c=" "; if (n=="y") printf("%s%c", tolower($i), c); else printf("%s%c", $i, c); } }' > tmp

if [ ! -e $ANALOGY_VEC ]
then
    #create the vocab
    cat tmp | awk -v n=$NORMALIZE '{for (i=1;i<=NF;i++) if (n=="y") print tolower($i); else print $i}'|sort|uniq > $ANALOGY_NAME.vocab
    echo "Saving the context vecs for the analogy words"
    python saveptvecs.py $ANALOGY_NAME.vocab $ANALOGY_VEC
fi

#append the EOF
echo EXIT >> tmp

cp /dev/null nohup.out
nohup $BIN/wordanalogy $ANALOGY_VEC < tmp

rm tmp
tail -n1 nohup.out

