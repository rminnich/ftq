#!/bin/bash
# Bulk welching for FTQ, customized for brho's environment.  YMMV.
#
# btw, here's a common line for running png.R
#
# for i in `seq 1 20`; do Rscript ~/akaros/ftq/scripts/png.R -i $i.dat -o $i.png --ymin=400 --ymax=900; done

for infile in "$@"
do
	outfile=`echo $infile | sed 's/.dat$//'`
	Rscript ~/akaros/ftq/scripts/welch.R -i $infile -o $outfile-100.pdf --xmax=100
	Rscript ~/akaros/ftq/scripts/welch.R -i $infile -o $outfile-500.pdf --xmax=500
	Rscript ~/akaros/ftq/scripts/welch.R -i $infile -o $outfile-1000.pdf --xmax=1000
	Rscript ~/akaros/ftq/scripts/welch.R -i $infile -o $outfile-5000.pdf --xmax=5000
done
