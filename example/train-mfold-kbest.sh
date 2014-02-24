#!/bin/bash
set -e

SOURCE_IN=data/train.en
TARGET_IN=data/train.ja
ALIGN_IN=data/train.en-ja.align
NUM_FOLD=4
LOSS_PROFILE="chunk=0.5|tau=0.5"
FEATURE_PROFILE="\
seq=dict=output/train.en-ja.pt,D%0QE0%aT,D0%0Q#00%aT,D1%0Q#01%aT,D2%0Q#02%aT,\
Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,BIAS%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
|cfg=LP0%l0%aT,RP0%r0%aT,SP0%s0%aT,TP0%s0%l0%r0%aT,LP1%l1%aT,RP1%r1%aT,SP1%s1%aT,TP0%s1%l1%r1%aT,SP2%s2%aT"
MAX_STATE=3
THREADS=4
SHUFFLE=false
ITERATION=10
VERBOSE=0
UPDATE=max
BEAM=10
MAX_TERM=3

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

# This bash file provides an example of how to train a model for lader.
# There are a couple steps, some of which are optional.
#
# This is just a toy example, but there is also a full working example
# for the Kyoto Free Translation task at http://www.phontron.com/kftt

[[ -e output ]] || mkdir output

#############################################################################
# 1. Creating annotations for each fold
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

run "../script/add-classes.pl data/classes.en < $SOURCE_IN > output/train.en.class"

#echo "../script/add-classes.pl data/classes.en < data/test.en > output/test.en.class"
#../script/add-classes.pl data/classes.en < data/test.en > output/test.en.class

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

#echo "../script/contiguous-extract.pl $SOURCE_IN $TARGET_IN $ALIGN_IN > output/train.en-ja.pt"
#../script/contiguous-extract.pl $SOURCE_IN $TARGET_IN $ALIGN_IN > output/train.en-ja.pt

#############################################################################
# 2. Combining annotations
#
# Once we have all our annotations, we want to combine them all into a single
# big file for training. This can be done with the unix "paste" command. Note
# that this, of course, does not apply to the phrase table. We will add this
# later.

run "paste $SOURCE_IN output/train.en.class $SOURCE_IN.pos $SOURCE_IN.parse > output/train.en.annot"

#############################################################################
# 3. Creating m folds
LINE_IN=$(wc -l output/train.en.annot | cut -f1 -d ' ')
LINES=$(python -c "from math import ceil; print int(ceil($LINE_IN.0/$NUM_FOLD))")
SUFFIX=$(echo $NUM_FOLD | wc -c)

rm -rf output/tmp
mkdir -p output/tmp

for i in output/train.en.annot $SOURCE_IN $TARGET_IN $ALIGN_IN; do

filename=$(basename "$i")

split -l $LINES -d -a $SUFFIX $i output/tmp/$filename.

done

FOLD_END=$(echo $NUM_FOLD - 1 | bc)
for m in $(seq -f "%0"$SUFFIX"g" 0 $FOLD_END); do
	mkdir -p output/fold$m
	train=$(seq -f "%0"$SUFFIX"g" 0 $FOLD_END | sed "s/$m//g")
	for i in output/train.en.annot $SOURCE_IN $TARGET_IN $ALIGN_IN; do
		filename=$(basename "$i")
		rm -f output/fold$m/$filename
		for j in $train; do
			cat output/tmp/$filename.$j >> output/fold$m/$filename
		done
	done
	run "../script/contiguous-extract.pl output/fold$m/train.en output/fold$m/train.ja \
				output/fold$m/train.en-ja.align > output/fold$m/train.en-ja.pt"
done

#############################################################################
# 3. Training
#
# Finally, we can train a model. 
# In order to do this, we must first define a feature set and set it with the
# -feature_profile option. You can find more info in doc/features.html, but if 
# you want to replicate the feature set in the paper, you can specify in the
# order of your annotations:
#
# ***** for the actual sentence *****
# seq=dict=output/train.en-ja.pt,LL%SL%ET,RR%SR%ET,LR%LR%ET,RL%RL%ET,O%SL%SR%ET,I%LR%RL%ET,Q%SQE0%ET,Q0%SQ#00%ET,Q1%SQ#01%ET,Q2%SQ#02%ET,CL%CL%ET,B%SB%ET,A%SA%ET,N%SN%ET,BIAS%ET
#
# Note that here we are using the phrase table in "dict". Change this path into
# an absolute path if you want to use the model from any other directory than
# the current one.
# 
# ***** for all tag-based annotations (class, POS) *****
# seq=LL%SL%ET,RR%SR%ET,LR%LR%ET,RL%RL%ET,B%SB%ET,A%SA%ET,O%SL%SR%ET,I%LR%RL%ET
#
# ***** for the parse *****
# cfg=LP%LP%ET,RP%RP%ET,SP%SP%ET,TP%SP%LP%RP%ET
#
# In this case, we will be using the words/classes/pos/parse, so we do one of
# the first, two of the second, and one of the third, in order, split by 
#
# There are also a couple of other options that you can tune for greater
# performance. (more in "train-shift-reduce --help")
# 
# -attach_null left/right (should be right (default) for English, as we want to
#                          attach null-aligned articles to the right word)
#
# -cost ...   (regularization cost. should be tuned based on data size, but
#              the default 1e-3 works pretty well in general)
#
# -save_features ... (default is true which uses more memory but runs slower.
#                     if you are using large data, this should be used with -features_dir)
# -features_dir ...	(store features on disk instead of in memory.)
# -threads ...	(the number of threads used for parallel feature generation, parallel cube pruning/growing at cell-level
# -cube_growing ...	(default is false which uses a lazy search)

for m in $(seq -f "%0"$SUFFIX"g" 0 $FOLD_END); do


#run "../src/bin/train-shift-reduce -cost 1e-3 -attach_null right \
#-loss_profile '$LOSS_PROFILE' -feature_profile '$FEATURE_PROFILE' \
#-iterations $ITERATION -threads $THREADS -shuffle $SHUFFLE -verbose $VERBOSE \
#-model_in $MODEL_IN'' -model_out output/fold$m/train.mod \
#-source_in output/fold$m/train.en.annot -align_in output/fold$m/train.en-ja.align \
#-update $UPDATE -beam $BEAM -max_state $MAX_STATE -max_term $MAX_TERM \
#-source_dev output/tmp/train.en.annot.$m -align_dev output/tmp/train.en-ja.align.$m \
#> output/fold$m/train.out 2> output/fold$m/train.log"

run "../src/bin/shift-reduce-kbest -model output/fold$m/train.mod \
-out_format score,flatten -threads $THREADS -beam $BEAM -max_state $MAX_STATE \
-verbose $VERBOSE -source_in output/tmp/train.en.annot.$m \
> output/fold$m/kbest.out 2> output/fold$m/kbest.log"

done
# Once training finishes, a reordering model will be placed in output/train.mod.
# This can be used in reordering, as described in run-reordering.sh
