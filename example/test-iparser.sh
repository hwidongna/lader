#!/bin/bash
set -e

TEST_IN=test.en
SOURCE_IN=output/test.en.annot
TARGET_IN=data/test.ja
ALIGN_IN=data/test.en-ja.align
MODEL_IN=output/train.iparser.mod
OUTPUT=output/test.en.iparser
THREADS=2
BEAM=10
VERBOSE=1

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
# This bash file provides an example of how to run intermediate parser and evaluate its
# accuracy. Before using this file, you must run train-iparser.sh to create
# the model to be used.

#############################################################################
# 1. Creating/combining annotations

run "../script/add-classes.pl data/classes.en < data/$TEST_IN > output/$TEST_IN.class"

run "paste data/$TEST_IN output/$TEST_IN.class data/$TEST_IN.pos data/$TEST_IN.parse > $SOURCE_IN"
#############################################################################
# 2. Running the reorderer

run "../src/bin/iparser -model $MODEL_IN -out_format order,string,flatten,action -threads $THREADS -beam $BEAM -verbose $VERBOSE -source_in $SOURCE_IN > $OUTPUT 2> $OUTPUT.log"

#############################################################################
# 3. Evaluating the reordered output

t=test
# source to target direction
run "../src/bin/iparser-gold -verbose $VERBOSE \
-out_format action,order,string,flatten -attach_null right \
-source_in data/$t.en -align_in data/$t.en-ja.align \
> output/$t.en-ja.gold.out 2> output/$t.en-ja.gold.log"

# get alignment in reverse direction
run "cut -f4 data/$t.en-ja.align -d'|' | sed 's/ //1' > data/$t.align"
run "../script/make-alignments.pl -reverse data/$t.{ja,en,align} data/$t.ja-en.{ja,en,align}"

# target to source direction
run "../src/bin/iparser-gold -verbose $VERBOSE -attach_null left \
-out_format action,order,string,flatten \
-source_in data/$t.ja -align_in data/$t.ja-en.align \
> output/$t.ja-en.gold.out 2> output/$t.ja-en.gold.log"

LOSS_PROFILE="levenshtein=1"
run "../src/bin/evaluate-iparser -loss_profile '$LOSS_PROFILE' \
-attach_null right -attach_trg left \
$ALIGN_IN $OUTPUT data/$TEST_IN $TARGET_IN'' \
> output/$TEST_IN.iparser.grade 2> output/$TEST_IN.iparser.log"

tail -n3 output/$TEST_IN.iparser.grade
	
