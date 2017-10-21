#!/bin/bash

RUN=$1
ZERORUN=$(printf "%06d" $RUN)

echo "Prepare environment"

MARLIN_DLL=
source /home/readout/ILCSoft/v01-19-02/Eutelescope/master/build_env.sh
MARLIN_DLL=$MARLIN_DLL:/home/readout/ALiBaVa/cautious-lamp/build/libAlibavaDataMerger.so

#echo "Merge telescope run $RUN"
#cd ~/alibava
#jobsub.py -c config.cfg alibavadatamerger $RUN

echo "Start DQM for alibava and telescope run $RUN"
~/ALiBaVa/cautious-lamp/build/alibavacorrelator -f ~/alibava/output/root/run$ZERORUN.root -o ~/ALiBaVa/dqmOutput/corr$ZERORUN



