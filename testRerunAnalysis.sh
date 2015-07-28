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
	outSubDir="analysis"
fi

mkdir -p $inDir/$outSubDir

for f in $inDir*.h264 
do
	echo "processing $f"
	# get dataset name
	outFolder=$(dirname $f)/$outSubDir/$(basename $f .h264)
	# program call
	program="./h264-mv-extractor $f -l tmpLog.dat -a $outFolder"
	echo "$program"
	eval $program
done

rm tmpLog.dat

