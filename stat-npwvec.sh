if [ $# -lt 1 ]
then
	echo "Usage: $0 <vec file>"
	exit
fi

echo "Computing maximum number of dimensions"
maxdim=`cat $1 | awk '{print $3}' |sort -nr|head -n1`
echo "maxdim = $maxdim"


echo "Computing stats..."
cat $1 | awk -v maxdim=$maxdim '\
function ceil(x) { y=int(x); return(x>y?y+1:y) } \
{ \
s+=(maxdim-$3); d+=$3} END{\
avgdim = d/NR; \
printf("#compression-ratio = %.4f average-#dim = %.4f ceil(avg-dim) = %d\n", s/(maxdim*NR), avgdim, ceil(avgdim));\
}'
