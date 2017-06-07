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

datamerger ${DQM_LCIO_PATH}/run${telrun}-hitmaker.slcio \
${KIT_MAPSA_DATA_PATH}/run${mparun}.root \
${MERGED_PATH}/merged${mparun}.root \
&& correlator ${MERGED_PATH}/merged${mparun}.root \
${CORRELATED_PATH}/corr${mparun}.root swap
mv ${CORRELATED_PATH}/corr${mparun}.root*.png ${IMAGE_PATH}
