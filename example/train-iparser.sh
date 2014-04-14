#!/bin/bash
set -e

SOURCE_IN=output/train.en.annot
TARGET_IN=data/train.ja
ALIGN_IN=data/train.en-ja.align
SOURCE_DEV=output/test.en.annot
ALIGN_DEV=data/test.en-ja.align
MAX_STATE=3
MAX_INS=3
MAX_DEL=3
THREADS=4
SHUFFLE=false
ITERATION=10
VERBOSE=1
UPDATE=max
BEAM=10

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

# This bash file provides an example of how to train a model for lader.
# There are a couple steps, some of which are optional.
#
# This is just a toy example, but there is also a full working example
# for the Kyoto Free Translation task at http://www.phontron.com/kftt

[[ -e output ]] || mkdir output

#############################################################################
# 1. Creating annotations
#  First, if we want to use features that reference word classes, pos tags,
#  parse trees, or phrase tables, we will need to create them in the appropriate
#  format. Note that all of these are optional, but all have a chance to improve
#  accuracy. Particularly word classes are calculated during alignment with
#  GIZA++, and phrase tables can be calculated trivially with the included
#  script, so you might as well use them.
#
#  a) Word classes
#   If we have a word class file in the form
#   > word1 class1
#   > word2 class2
#   we can use script/add-classes.pl to annotate a corpus with its corresponding
#   classes. For example, the following command will create a file with the
#   words in data/train.en annotated with their classes in data/classes.en

run "../script/add-classes.pl data/classes.en < data/train.en > output/train.en.class"

run "../script/add-classes.pl data/classes.en < data/test.en > output/test.en.class"

#  b) POS tags
#   You can also add POS tags in a similar fashion using your favorite tagger.
#   An example of what this would look like is included in data/train.en.pos
#
#  c) Parse trees
#   You can also define parse trees in Penn treebank format in the feature set.
#   An example of these is shown in data/train.en.parse.
#
#  d) Phrase table
#   Phrase table features help prevent good phrases from being broken apart
#   in the parse. The phrase table can be extracted from source sentences
#   and alignments using the script/phrase-extract.pl command. (By default this
#   only extracts phrases that appear more than once, which can be changed with
#   the -discount option.)

run "../script/contiguous-extract.pl data/train.en data/train.ja $ALIGN_IN > output/train.en-ja.pt"

#############################################################################
# 2. Combining annotations
#
# Once we have all our annotations, we want to combine them all into a single
# big file for training. This can be done with the unix "paste" command. Note
# that this, of course, does not apply to the phrase table. We will add this
# later.

run "paste data/train.en output/train.en.class data/train.en.pos data/train.en.parse > $SOURCE_IN"

run "paste data/test.en output/test.en.class data/test.en.pos data/test.en.parse > $SOURCE_DEV"

#############################################################################
# 3. Generate gold action sequences
#
# For intermediate trees containing delete/insert actions
# bidirectional gold action sequences are obtained
for t in train 'test'; do

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

done

#############################################################################
# 4. Training
#
# Finally, we can train a model. 
# Utilize the bidirectional gold action sequences
# Instead of directly using alignment information
# Thus, loss functions are also defined over gold action sequences
# e.g. edit distance, affine gap distance (to be considered), etc.

LOSS_PROFILE="levenshtein=0.5|affine=0.5"
FEATURE_PROFILE="\
seq=dict=output/train.en-ja.pt,D%0QE0%aT,D0%0Q#00%aT,D1%0Q#01%aT,D2%0Q#02%aT,\
Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,BIAS%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
|cfg=LP0%l0%aT,RP0%r0%aT,SP0%s0%aT,TP0%s0%l0%r0%aT,LP1%l1%aT,RP1%r1%aT,SP1%s1%aT,TP0%s1%l1%r1%aT,SP2%s2%aT"

run "../src/bin/train-iparser -cost 1e-3 \
-loss_profile '$LOSS_PROFILE' -feature_profile '$FEATURE_PROFILE' \
-iterations $ITERATION -threads $THREADS -shuffle $SHUFFLE \
-verbose $VERBOSE -model_in $MODEL_IN'' -model_out output/train.iparser.mod \
-source_in $SOURCE_IN \
-source_gold output/train.en-ja.gold.out \
-target_gold output/train.ja-en.gold.out \
-source_dev $SOURCE_DEV \
-source_dev_gold output/test.en-ja.gold.out \
-target_dev_gold output/test.ja-en.gold.out \
-update $UPDATE -beam $BEAM -max_state $MAX_STATE \
-max_ins $MAX_INS -max_del $MAX_DEL -verbose $VERBOSE"

# Once training finishes, a reordering model will be placed in output/train.iparser.mod.
# This can be used in reordering, as described in test-iparser.sh
