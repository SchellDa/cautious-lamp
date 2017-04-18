#!/bin/bash

# Flag to check if env is sourced
export TBENV_INIT=1

# Raw data paths
export DQM_LCIO_PATH="/home/readout/tbAnalysis/output/lcio"
export KIT_MAPSA_DATA_PATH="/home/readout/DaTeN/mpa"
export HEPHY_MAPSA_DATA_PATH="/home/readout/hephyData"

# Base output path
export TESTBEAM_PATH="/home/readout/TestBeam2017"

# Output paths
export MERGED_PATH="${TESTBEAM_PATH}/kit/merged"
export MERGED_HEPHY_PATH="${TESTBEAM_PATH}/hephy/merged"
export SHIFTED_PATH="${TESTBEAM_PATH}/kit/shifted"
export SHIFTED_HEPHY_PATH="${TESTBEAM_PATH}/hephy/shifted"
export CORRELATED_PATH="${TESTBEAM_PATH}/kit/correlated"
export CORRELATED_HEPHY_PATH="${TESTBEAM_PATH}/hephy/correlated"
export IMAGES_PATH="${TESTBEAM_PATH}/kit/images"
export IMAGES_HEPHY_PATH="${TESTBEAM_PATH}/hephy/images"
echo "Loaded Testbeam 2017 Environment"
