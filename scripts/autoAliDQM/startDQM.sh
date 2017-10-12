#!/bin/bash

ALIRUN=$1
TELRUN=$2
MERGED="a$ALIRUN""_t$TELRUN"".root"
OUT=$3

echo "Eutelescope stuff $TELRUN"
sleep 10
echo "Merge alibava run $ALIRUN with telescope run $TELRUN -> $MERGED"
sleep 10
echo "Correlate $MERGED"
