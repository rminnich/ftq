#!/bin/bash
HOSTS="XXX YYY"

for h in $HOSTS; do
	echo ======= Syncing from $h ========
	rsync -avP -e ssh root@$h:/tmp/b/ftq_data/* ../ftq_data/
done
