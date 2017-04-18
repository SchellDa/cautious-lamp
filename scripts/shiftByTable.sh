#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 CSV-FILE"
	echo ""
	echo "A CSV file with three columns (MPA-RUN REF-SHIFT TEL-SHIFT)"
	echo "is expected by this script."

counter=0
csvfile=$1
for line in `cat $csvfile`; do
	IFS=","
	set -- $line
	echo "Start Run $1"
	if [ `expr $counter % 2` -eq 0 ]; then
		runShift.sh $1 $2 $3 &
	else
		runShift.sh $1 $2 $3
	fi
	counter=`expr $counter + 1`
done
