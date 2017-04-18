#!/bin/bash -l
checkTbEnv.sh && exit 1
if [ $# -ne 0 ] && [ $# -ne 3 ]; then
	echo "Usage: $0 [TEL-RUN KIT-RUN HEPHY-RUN]"
	echo ""
	echo "Automaticaly analyse runs after acquiring them. If a tripplet"
	echo "of run numbers is given, the script also executes further"
	echo "analysis steps automaticaly."
	exit 1
fi

DO_MPA=0
if [ $# -eq 3 ]; then
	DO_MPA=1
	KitOffset=$((10#$2-10#$1))
	HephyOffset=$((10#$3-10#$1))
fi

while true; do
	analysisNumber=`ls -1r /home/readout/telescopeData/ | head -n4 | tail -n1 | sed "s/[\.a-z]//g"`
	lcioPath="/home/readout/tbAnalysis/output/lcio/run${analysisNumber}-clustering.slcio"
	if [ -f $lcioPath ]; then
		echo "LCIO file already exists, waiting..."
		sleep 10
		continue
	fi
	echo "Analyse Telescope Run $analysisNumber"
	prepare_telrun.sh $analysisNumber
	if [ $DO_MPA -eq 1 ]; then
		KitRun=$((10#$analysisNumber+$KitOffset))
		HephyRun=$((10#$analysisNumber+$HephyOffset))
		echo $((10#${analysisNumber})),${KitRun},${HephyRun} >> ${TESTBEAM_PATH}/auto_runnumbers.csv
		do_both.sh 10#$analysisNumber $KitRun $HephyRun &
	fi
	sleep 1
done
