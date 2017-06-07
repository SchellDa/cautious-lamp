#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 CSV-FILE"
	echo ""
	echo "A CSV file with three columns (MPA-RUN REF-SHIFT TEL-SHIFT)"
	echo "is expected by this script."
	exit 1
fi

counter=0
csvfile=$1
cmdfile=`mktemp`
for line in `cat $csvfile`; do
	IFS=","
	set -- $line
#	echo "Start Run $1"
#	if [ `expr $counter % 2` -eq 0 ]; then
#		runShift.sh $1 $2 $3 &
#	else
#		runShift.sh $1 $2 $3
#	fi
	if [ -z $2 ] || [ -z $3 ]; then
		continue;
	fi
	if [ $2 == 'x' ] || [ $3 == 'x' ] || [[ $2 == *"/"* ]] || [[ $3 == *"/"* ]]; then
		continue;
	fi
	echo "runShift.sh $1 $2 $3 > /dev/null 2>&1" >> $cmdfile
	counter=`expr $counter + 1`
done

echo "Cmd File " $cmdfile
# batchsubmit < $cmdfile
