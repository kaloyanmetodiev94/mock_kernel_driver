#!/bin/sh

if [ $# -eq 0 ]
  then
    echo "Please give .ko file as input: ./loadmodule.sh <filename>.ko"

  else
	sudo insmod $1
	base=$(basename $1 .ko)
#	sudo mknod /dev/$base c 249 0
#	sudo chmod 777 /dev/$base
fi
