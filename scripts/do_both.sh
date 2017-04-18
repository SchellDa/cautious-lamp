#!/bin/bash
checkTbEnv.sh && exit 1

if [ $# -ne 3 ]; then
	echo "Usage: $0 TelRunNo KitRunNo HephyRunNo"
	echo ""
	echo "This program does shit."
	exit 1
fi

do_shit.sh $1 $2 &
do_johannes.sh $1 $3

