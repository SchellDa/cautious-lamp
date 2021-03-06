#!/bin/bash
if [ $# -ne 3 ]; then
	echo "Usage: $0 RUN REF-SHIFT TEL-SHIFT"
	exit 1
fi
run=`printf %04i $(($1))`
shifter ${MERGED_PATH}/merged${run}.root ${SHIFTED_PATH}/shifted${run}.root $2 $3 \
&& correlator ${SHIFTED_PATH}/shifted${run}.root ${CORRELATED_PATH}/scorr${run}.root swap 0
mv ${CORRELATED_PATH}/scorr${run}.root*.png ${IMAGE_PATH}
