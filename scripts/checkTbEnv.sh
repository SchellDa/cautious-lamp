#!/bin/bash

if [ $TBENV_INIT ]; then
	exit 1
else
	echo "Source Testbeam Environment first"
	exit 0;
fi
