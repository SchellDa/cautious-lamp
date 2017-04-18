#!/bin/bash

checkTbEnv.sh && exit 1
if [ $# -ne 3 ]; then
	echo "Usage: $0 RUN REF-SHIFT TEL-SHIFT"
	exit 1
fi
run=`printf %04i $(($1))`
shifter ${MERGED_HEPHY_PATH}/Jmerged${run}.root ${SHIFTED_HEPHY_PATH}/Jshifted${run}.root $2 $3 \
&& correlator ${SHIFTED_HEPHY_PATH}/Jshifted${run}.root ${CORRELATED_HEPHY_PATH}/Jscorr${run}.root swap 0
mv ${CORRELATED_HEPHY_PATH}/Jscorr${run}.root*.png ${IMAGE_HEPHY_PATH}
