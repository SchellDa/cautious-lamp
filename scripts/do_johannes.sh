#!/bin/bash
checkTbEnv.sh && exit 1
if [ $# -ne 2 ]; then
	echo "Usage: $0 TelRunNo MpaRunNo"
	echo ""
	echo "This program does shit."
	exit 1
fi

telrun=`printf "%06i" $(($1))`
mparun=`printf "%04i" $(($2))`

datamerger ${DQM_LCIO_PATH}/run${telrun}-clustering.slcio \
${HEPHY_MAPSA_DATA_PATH}/run${mparun}.root \
${MERGED_HEPHY_PATH}/Jmerged${mparun}.root \
&& correlator ${MERGED_HEPHY_PATH}/Jmerged${mparun}.root \
${CORRELATED_HEPHY_PATH}/Jcorr${mparun}.root swap
mv ${CORRELATED_HEPHY_PATH}/Jcorr${mparun}.root*.png ${IMAGE_HEPHY_PATH}
