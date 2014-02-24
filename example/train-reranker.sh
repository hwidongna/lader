#!/bin/bash
set -e

SOURCE_IN=data/train.en
TARGET_IN=data/train.ja
ALIGN_IN=data/train.en-ja.align

THRESHOLD=5
THREADS=4
VERBOSE=0

# define helper function: run a command and print its exit code
function run () {
    echo -e "\nrun: $1\n-------------"
    eval $1
    local code=$?
    if [ $code -ne 0 ]; then
	echo "Exit code: $code"
	exit $code
    fi
}


run "cat output/fold*/kbest.out | \
../src/bin/extract-feature -threads $THREADS -verbose $VERBOSE \
-model_out output/feature.model -threshold $THRESHOLD \
> output/features.out 2> output/features.log"
