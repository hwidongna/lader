#!/bin/bash
set -e

BLLIP=~/git/bllip-parser/second-stage/programs
SOURCE_IN=data/train.en
TARGET_IN=data/train.ja
ALIGN_IN=data/train.en-ja.align

THRESHOLD=5
THREADS=4
VERBOSE=0
BEST_FOLD=03 # use the best fold
MAX_STATE=3
BEAM=10

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



# Extract features for train

run "cat output/fold*/gold.out > output/gold.train"
run "cat output/fold*/kbest.out | \
../src/bin/extract-feature -verbose $VERBOSE \
-gold_in output/gold.train -model_out output/features.model \
> output/features.train 2> output/features.train.log"
run "cat output/features.train | \
../src/bin/renumber-feature -model_in output/features.model \
-model_out output/features.renumber | sed 's/[ ][ ]*/ /g' | \
gzip -c > output/features.train.gz"

# Estimate feature weights
run "grep ^[0-9] output/features.renumber | cut -f1,2 |  gzip -c > output/features.gz"
run "zcat output/features.train.gz | \
$BLLIP/wlle/cvlm-lbfgs -l 1 -c 10 -F 1 -n -1 -p 2 \
-f output/features.gz \
-o output/features.train.weights"
