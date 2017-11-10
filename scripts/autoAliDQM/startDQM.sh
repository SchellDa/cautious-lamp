#!/bin/bash

#ALIRUN=$1
TELRUN=$1
#MERGED="a$ALIRUN""_t$TELRUN"".root"
#OUT=$3
CONFIG="config.cfg"
echo "Prepare environment"

MARLIN_DLL=
source /home/readout/ILCSoft/v01-19-02/Eutelescope/master/build_env.sh
MARLIN_DLL=$MARLIN_DLL:/home/readout/ALiBaVa/cautious-lamp/build/libAlibavaDataMerger.so

echo "Prepare telescope run $TELRUN"
cd ~/alibava
#jobsub.py -c $CONFIG converter $TELRUN
#jobsub.py -c $CONFIG clustering $TELRUN
#jobsub.py -c $CONFIG hitmaker $TELRUN
jobsub.py -c $CONFIG alibavadatamerger $TELRUN

ZEROTELRUN=$(printf "%06d" $TELRUN)

echo "Merge alibava run $ALIRUN with telescope run $TELRUN"
~/ALiBaVa/cautious-lamp/build/alibavacorrelator -f ~/alibava/output/root/run$ZEROTELRUN.root -o ~/ALiBaVa/dqmOutput/corr$ZEROTELRUN

echo "Correlate data"

