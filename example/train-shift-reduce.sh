#!/bin/bash
set -e

SOURCE_IN=output/train.en.annot
TARGET_IN=data/train.ja
ALIGN_IN=data/train.en-ja.align
SOURCE_DEV=output/test.en.annot
ALIGN_DEV=data/test.en-ja.align
MODEL_OUT=output/train.srdp.mod
LOSS_PROFILE="chunk=1.0" #"chunk=0.5|tau=0.5"
FEATURE_PROFILE="\
seq=dict=output/train.en-ja.pt,D%0QE0%aT,D0%0Q#00%aT,D1%0Q#01%aT,D2%0Q#02%aT,\
Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,B0%s0B%aT,A0%s0A%aT,B1%s1B%aT,A1%s1A%aT,BIAS%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT,B0%s0B%aT,A0%s0A%aT,B1%s1B%aT,A1%s1A%aT,B2%s2B%aT,A2%s2A%aT\
|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT,B0%s0B%aT,A0%s0A%aT,B1%s1B%aT,A1%s1A%aT,B2%s2B%aT,A2%s2A%aT\
|cfg=LP0%l0%aT,RP0%r0%aT,SP0%s0%aT,TP0%s0%l0%r0%aT,LP1%l1%aT,RP1%r1%aT,SP1%s1%aT,TP0%s1%l1%r1%aT,SP2%s2%aT"
#################### non-local features ########################
#FEATURE_PROFILE="\
#seq=dict=output/train.en-ja.pt,D%0QE0%aT,D0%0Q#00%aT,D1%0Q#01%aT,D2%0Q#02%aT,\
#Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,BIAS%aT\
#,TLS%t1R%t0L%aT,TLI%t0R%t1L%aT\
#|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
#,TLS%t1R%t0L%aT,TLI%t0R%t1L%aT\
#|seq=Q0%q0%aT,Q1%q1%aT,Q2%q2%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,LL1%s1L%aT,RR1%s1R%aT,LR1%l1R%aT,RL1%r1L%aT,O1%s1L%s1R%aT,I1%l1R%r1L%aT,LL2%s2L%aT,RR2%s2R%aT,O2%s2L%s2R%aT\
#,TLS%t1R%t0L%aT,TLI%t0R%t1L%aT\
#|cfg=LP0%l0%aT,RP0%r0%aT,SP0%s0%aT,TP0%s0%l0%r0%aT,LP1%l1%aT,RP1%r1%aT,SP1%s1%aT,TP0%s1%l1%r1%aT,SP2%s2%aT"
MAX_STATE=3
THREADS=4
SHUFFLE=false
ITERATION=10
VERBOSE=1
LEARNER=pegasos  
UPDATE=none
BEAM=10
MAX_TERM=1
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

#run "../script/add-classes.pl data/classes.en < data/test.en > output/test.en.class"
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

run "../script/contiguous-extract.pl data/train.en data/train.ja $ALIGN_IN > output/train.en-ja.pt"

#############################################################################
# 2. Combining annotations
#
# Once we have all our annotations, we want to combine them all into a single
# big file for training. This can be done with the unix "paste" command. Note
# that this, of course, does not apply to the phrase table. We will add this
# later.

run "paste data/train.en output/train.en.class data/train.en.pos data/train.en.parse > $SOURCE_IN"

#run "paste data/test.en output/test.en.class data/test.en.pos data/test.en.parse > $SOURCE_DEV"
#paste data/test.en output/test.en.class data/test.en.pos data/test.en.parse > $SOURCE_DEV
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

run "../src/bin/train-shift-reduce -cost 1e-3 -attach_null right -loss_profile '$LOSS_PROFILE' -feature_profile '$FEATURE_PROFILE' -iterations $ITERATION -threads $THREADS -shuffle $SHUFFLE -verbose $VERBOSE -model_in $MODEL_IN'' -model_out $MODEL_OUT -source_in $SOURCE_IN -align_in $ALIGN_IN -update $UPDATE -source_dev '$SOURCE_DEV' -align_dev '$ALIGN_DEV' -beam $BEAM -max_state $MAX_STATE -max_term $MAX_TERM -verbose $VERBOSE -max_swap $MAX_SWAP -learner $LEARNER"

# Once training finishes, a reordering model will be placed in output/train.mod.
# This can be used in reordering, as described in run-reordering.sh
