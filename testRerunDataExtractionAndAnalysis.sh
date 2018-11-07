#!/bin/bash

echo "h264 test data collection" 
echo "extracts motion vector data per frame from every H264 file found in given input folder" 

usage() {
	echo "Usage: ${0} INPUT_DIR [OUTPUT_SUBFOLDER] [ARGS]"
	exit 1
}

inDir=$1
if [ ! -d "${inDir}" ]; then
	echo "input directory: ${inDir} not found"
	usage;
fi
echo "input: ${inDir}"

if [ "$#" -ge "2" ]; then
	outSubDir=$2
else
	outSubDir="data"
fi

if [ "$#" -ge "3" ]; then
	args=$3
else
	args=""
fi

mkdir -p ${inDir}/${outSubDir}

for f in ${inDir}/*.h264; do
	[ -e "${f}" ] || continue

	echo "processing ${f}"
	# get dataset name
	outData=$(dirname $f)/${outSubDir}/$(basename $f .h264).dat
	# set directory
	analysisDir=$(dirname $f)/${outSubDir}/analysis/$(basename $f .h264)
	mkdir -p ${analysisDir}
	# program call
	program="./h264-mv-extractor ${args} -i $f -f ${outData} -e${analysisDir}"
	echo "${program}"
	eval ${program}
done
