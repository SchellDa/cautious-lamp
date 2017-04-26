#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 CSV-FILE"
	echo ""
	echo "A CSV file with three columns (TEL-RUN MPA-UP-RUN MPA-DOWN-RUN)"
	echo "is expected by this script."
	exit 1
fi

csvfile=$1
cmdfile=`mktemp`
for line in `cat $csvfile`; do
	IFS=","
	set -- $line
	if [ $2 = "X" ]; then
		continue;
	fi
	echo "do_shit.sh $1 $2 > /dev/null 2>&1" >> $cmdfile
done

echo "Cmd File " $cmdfile
batchsubmit < $cmdfile

