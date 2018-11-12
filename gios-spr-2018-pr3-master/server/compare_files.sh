#!/bin/bash

if [ -z $2 ]; then
    echo "Usage: compare_files.sh <source> <workloadfile>"
    exit
fi

# Curl each file in workload.txt into a new 'clean' dir
mkdir ./clean
cd ./clean
cat ../$2 | while read line; do
    curl -sS --create-dirs $1$line -o .$line
done
cd ..

# Compares client's files to clean files and outputs if they differ
cat $2 | while read line; do
    out=`diff -q ./clean$line .$line`
    if [ -n "$out" ]; then
        echo $out
    fi
done