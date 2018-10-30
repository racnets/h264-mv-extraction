#!/bin/bash
probe1="./testRerunDataExtraction.sh"
probe2="./testRerunDataExtractionAndAnalysis.sh"
probeName="rerunPiB3+"
experiments=("../videos/20140115_outside/"
	"../videos/20140131_inside/"
	"../videos/20140217_inside/"
	"../videos/20140305_inside_longterm/"
	)

for exp in "${experiments[@]}"
do
	program="${probe1} ${exp} ${probeName}"
	echo "${program}"
	eval $program
	program="${probe1} ${exp} ${probeName}_exp -x"
	echo "${program}"
	eval $program
	program="${probe1} ${exp} ${probeName}_ext -e"
	echo "${program}"
	eval $program
	program="${probe1} ${exp} ${probeName}_exp_ext -e -x"
	echo "${program}"
	eval $program
	program="${probe2} ${exp} ${probeName}_ana"
	echo "${program}"
	eval $program
	program="${probe2} ${exp} ${probeName}_exp_ana -x"
	echo "${program}"
	eval $program
done
