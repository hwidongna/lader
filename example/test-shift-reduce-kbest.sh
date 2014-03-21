#!/bin/bash
set -e

TEST_IN=test.en
SOURCE_IN=output/test.en.annot
TARGET_IN=data/test.ja
ALIGN_IN=data/test.en-ja.align
MODEL_IN=output/train.mod
OUTPUT=output/test.en.reordered
THREADS=2
BEAM=10
VERBOSE=1
MAX_SWAP=1

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
# 3. Running the reorderer
#
# Once we have our annotated input we can run the reorderer. To do so, we must
# simply specify the model file and pipe the file we want to reorder.
#
# The one thing we do need to decide is the output format. lader can output
# reordererd sentences ("string"), the latent parse tree ("parse") or the IDs
# of the reordered words in the original sentence. Let's output all of them
# for now.

run "../src/bin/shift-reduce-kbest -model $MODEL_IN -out_format score,flatten -threads $THREADS -beam $BEAM -verbose $VERBOSE -source_in $SOURCE_IN > $OUTPUT 2> $OUTPUT.log"

#############################################################################
# 4. Evaluating the reordered output
#
# If we have a set of alignments for the test set, we can evaluate the accuracy
# of the reorderer. This is done with evaluate-lader, which takes as input
# the true alignment, the reorderer output, and optionally the source and target
# sentences. Note that for evaluation, we must choose out_format for lader where
# "order" comes before any others (as above)
# 
# Also note that we need to set -attach-null to the same value that we set
# during training. (In this case, we'll use the default, "right")

run "cat output/kbest.dev | cut -f1,2 | \
../src/bin/reranker-oracle -attach_null right \
$ALIGN_IN data/$TEST_IN $TARGET_IN'' \
> output/$TEST_IN.grade 2> output/$TEST_IN.log" 

tail -n3 output/$TEST_IN.grade
	
