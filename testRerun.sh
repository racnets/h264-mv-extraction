#!/bin/bash

echo "h264 test data collection" 

inDir=$1
if [ ! -d "$inDir" ]; then
	echo "input directory: $inDir not found"
	exit
fi
echo "input: $inDir"

if [ "$#" -ge "2" ]; then
	outSubDir=$2
else
	outSubDir="rerun"
fi

mkdir -p $inDir/$outSubDir

for f in $inDir*.h264 
do
	echo "processing $f"
	# get dataset name
	inData=$(dirname $f)/$(basename $f .h264).dat
	outData=$(dirname $f)/$outSubDir/$(basename $f .h264).dat
	# program call
	program="./h264-mv-extractor -i $f -a -f $outData"
	echo "$program"
	eval $program
	# get header from dataset
	# and copy header to new dataset file
	header=$(head -n 3 $inData | tail -n 1)
	$(sed -i "1i $header" $outData)
	header=$(head -n 2 $inData | tail -n 1)
	$(sed -i "1i $header\t 1" $outData)
	header=$(head -n 1 $inData | tail -n 1)
	$(sed -i "1i $header\trerun" $outData)
done

