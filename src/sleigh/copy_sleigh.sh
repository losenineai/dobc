#!/bin/bash

filelist=`ls | xargs`
for file in $filelist
do
  cp $1/$file .
done
