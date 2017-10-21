#!/bin/bash

#ALIRUN=$1
TELRUN=$1
#MERGED="a$ALIRUN""_t$TELRUN"".root"
#OUT=$3

echo "Prepare environment"

MARLIN_DLL=
source /home/readout/ILCSoft/v01-19-02/Eutelescope/master/build_env.sh
MARLIN_DLL=$MARLIN_DLL:/home/readout/ALiBaVa/cautious-lamp/build/libAlibavaDataMerger.so

echo "Prepare telescope run $TELRUN"
cd ~/alibava
jobsub.py -c config.cfg converter $TELRUN
jobsub.py -c config.cfg clustering $TELRUN
jobsub.py -c config.cfg hitmaker $TELRUN
jobsub.py -c config.cfg alibavadatamerger $TELRUN

ZEROTELRUN=$(printf "%06d" $TELRUN)

echo "Merge alibava run $ALIRUN with telescope run $TELRUN"
~/ALiBaVa/cautious-lamp/build/alibavacorrelator -f ~/alibava/output/root/run$ZEROTELRUN.root -o ~/ALiBaVa/dqmOutput/corr$ZEROTELRUN

echo "Correlate data"

