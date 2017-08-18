#!/bin/bash
checkTbEnv.sh && exit 1
if [ $# -ne 1 ]; then
	echo "Usage: $0 MpaRunNo"
	echo ""
	echo "Perform Detailed-Timecorrelation-Shift scan."
	exit 1
fi

mparun=`printf "%04i" $(($1))`
mkdir -p ${IMAGES_PATH}/dts
dts ${MERGED_PATH}/merged${mparun}.root \
${IMAGES_PATH}/dts/dts${mparun}.png \
${IMAGES_PATH}/dts/dts${mparun}.root \
swap
