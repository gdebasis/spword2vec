if [ $# -lt 3 ]
then
        echo "usage: $0 <bin file> <analogy dataset> <case normalize (y/n)>"
        exit
fi

PTVEC=$1
ANALOGY=$2
NORMALIZE=$3

BIN=..
PTVEC_NAME=$(basename $PTVEC)
ANALOGY_NAME=$(basename $ANALOGY)
SUBVEC=$PTVEC_NAME.sub.bin

cat $ANALOGY| awk -v n=$NORMALIZE '{for (i=1;i<=NF;i++) { if (i==NF) c="\n"; else c=" "; if (n=="y") printf("%s%c", tolower($i), c); else printf("%s%c", $i, c); } }' > tmp
echo EXIT >> tmp

cat $ANALOGY | awk -v n=$NORMALIZE '{for (i=1;i<=NF;i++) if (n=="y") print tolower($i); else print $i}'|sort|uniq > $ANALOGY_NAME.vocab
$BIN/prune $PTVEC $ANALOGY_NAME.vocab $SUBVEC

$BIN/wordanalogy $SUBVEC < tmp

rm tmp


