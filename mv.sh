#!/bin/bash

# script to rename .out files to .sol files

for i in `seq 65 90`;
do
  c=$(printf \\$(printf "%o" $i))
  ls $c.out1 2> /dev/null | grep $c.out1 > /dev/null 2> /dev/null
  if [ "$?" != "0" ]; then
    break
  fi
  j=1
  while [ "1" = "1" ]
  do
    ls $c.out$j 2> /dev/null | grep $c.out$j > /dev/null 2> /dev/null
    if [ "$?" != "0" ]; then
      break
    fi
    mv $c.out$j $c.sol$j
    j=$((j+1))
  done
done
