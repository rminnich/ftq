#!/bin/bash
# bulk processing of ftq_data from another machine.
# fetch, make a png, and run FFTs
#
# run this from the root of the repo:  ./scripts/proc_data.sh

# edit accordingly
HOST_DATA="something.com:path/to/ftq_data/"

mkdir -p ftq_data/
rsync -avP -e ssh $HOST_DATA/* ftq_data/

for d in `find ./ftq_data/ -name '*.dat'`; do
	# drop the trailing .dat
	TRIM=${d%.*}
	PNG=${TRIM}.png
	if [ ! -f $PNG ]; then
		echo Missing $PNG
		Rscript ./scripts/png.R -i $d -o $PNG
	fi
	for f in 100 500 1000 5000; do
		PDF=${TRIM}-${f}.pdf
		if [ ! -f $PDF ]; then
			echo Missing $PDF
			Rscript ./scripts/welch.R -i $d -o $PDF --xmax=$f
		fi
	done
done
