#!/bin/bash
# Runs FTQ for a bunch of different binaries, stores the data in ftq_data.
#
# run this on the machine under test.

#BINS="ftq.clang ftq.gcc.baseline ftq.gcc.unroll ftq.gcc.noopt ftq.clang.noopt"
#BINS="ftq.static.linux"
BINS="ftq.gcc.noopt.float20"

FREQ=7777
CPU=16

mkdir -p ftq_data/

for b in $BINS; do
	echo "===== Running $b pinned on cpu $CPU"
	taskset -c $CPU ./$b -f $FREQ -o ftq_data/ftq_${HOSTNAME}_${b}_cpu${CPU}

	echo ""

	echo "===== Running $b unpinned"
	./$b -f $FREQ -o ftq_data/ftq_${HOSTNAME}_${b}

	echo ""
	echo ""
	echo ""
done
