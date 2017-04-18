#!/bin/bash 
#source ~/ILCsoft/v01-17-10/init_ilcsoft.sh
cd /home/readout/tbAnalysis
runlist_entry=`grep $1 runlist.csv`
if [ -z ${runlist_entry} ]; then
	echo "$1,gear_kit_00001.xml" >> runlist.csv
fi
jobsub -c config.cfg -csv runlist.csv converter $1 && jobsub -c config.cfg -csv runlist.csv clustering $1
