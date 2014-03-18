#!/bin/bash
set -e

BLLIP=~/git/bllip-parser/second-stage/programs
TEST_IN=test.en
SOURCE_IN=output/test.en.annot
TARGET_IN=data/test.ja
ALIGN_IN=data/test.en-ja.align
MODEL_IN=output/train.mod
FEATURE_IN=output/features.renumber
WEIGHTS=output/features.train.weights
OUTPUT=output/test.en.reordered
THREADS=2
BEAM=10
THRESHOLD=5
VERBOSE=0

# define helper function: run a command and print its exit code
function run () {
    echo -e "\nrun: $1\n-------------"
    eval $1
    local code=$?
    if [ $code -ne 0 ]; then
	run "Exit code: $code"
	exit $code
    fi
}
# This bash file provides an example of how to run lader and evaluate its
# accuracy. Before using this file, you must run train-model.sh to create
# the model to be used.

#############################################################################
# 1. Creating/combining annotations
#  Like train-model.sh, we need to create annotations for our input sentence.
#  This is the same as before, so read train-model.sh for more details.

run "../script/add-classes.pl data/classes.en < data/$TEST_IN > output/$TEST_IN.class"

run "paste data/$TEST_IN output/$TEST_IN.class data/$TEST_IN.pos data/$TEST_IN.parse > $SOURCE_IN"
#############################################################################
# 3. Running the reranker

# Generate the gold-standard tree for dev
run "../src/bin/gold-standard -verbose $VERBOSE \
-out_format action,order,string,flatten \
-source_in data/$TEST_IN -align_in $ALIGN_IN > output/gold.dev"

# Produce k-best for dev
run "../src/bin/shift-reduce-kbest -model $MODEL_IN \
-out_format score,action,order,string,flatten -threads $THREADS \
-beam $BEAM -verbose $VERBOSE -source_in $SOURCE_IN \
> output/kbest.dev 2> output/kbest.dev.log"

# Extract features for dev
run "cat output/kbest.dev | cut -f1,2 | \
../src/bin/extract-feature -verbose $VERBOSE 
-model_in $FEATURE_IN -threshold $THRESHOLD	\
-source_in data/$TEST_IN -gold_in output/gold.dev | \
../src/bin/renumber-feature -model_in output/features.model \
-model_out output/features.renumber | sed 's/[ ][ ]*/ /g' | \
gzip -c > output/features.dev.gz 2> output/features.dev.log"

# Evaluate weights
run "grep ^[0-9] $FEATURE_IN | cut -f1,2 |  gzip -c > output/features.gz"
run "cat $WEIGHTS | \
$BLLIP/eval-weights/eval-weights \
-a output/features.gz output/features.dev.gz > output/weights-eval"

# Run reranker
run "cat output/kbest.dev | cut -f1,2 | \
../src/bin/reranker -model_in $FEATURE_IN -weights $WEIGHTS \
-source_in data/$TEST_IN > output/$TEST_IN.reranker"

# Evaluate reranker result
run "../src/bin/evaluate-lader -attach_null right \
$ALIGN_IN output/$TEST_IN.reranker data/$TEST_IN $TARGET_IN'' \
> output/$TEST_IN.reranker.grade"

tail -n3 output/$TEST_IN.reranker.grade
	
# Run 1-best for comparison
run "cat output/kbest.dev | cut -f1,2 | \
../src/bin/reranker -model_in $FEATURE_IN -weights $WEIGHTS \
-source_in data/$TEST_IN -trees 1 > output/$TEST_IN.1best"

# Evaluate 1-best result
run "../src/bin/evaluate-lader -attach_null right \
$ALIGN_IN output/$TEST_IN.1best data/$TEST_IN $TARGET_IN'' \
> output/$TEST_IN.1best.grade" 

tail -n3 output/$TEST_IN.1best.grade

# Evaluate reranker oracle
run "cat output/kbest.dev | cut -f1,2 | \
../src/bin/reranker-oracle -attach_null right \
$ALIGN_IN data/$TEST_IN $TARGET_IN'' \
> output/$TEST_IN.oracle.grade" 

tail -n3 output/$TEST_IN.oracle.grade
