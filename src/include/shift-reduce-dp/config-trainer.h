#ifndef CONFIG_TRAINER_H__
#define CONFIG_TRAINER_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigTrainer : public ConfigBase {

public:

    ConfigTrainer() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ train-shift-reduce ~~~\n"
"  by Hwidong Na\n"
"\n"
"Trains a discriminative model for machine translation reordering using shift-reduce algorithm.\n"
);

        // AddConfigEntry("first_step", "1", "The first step (1=hypergraph, 2=loss, 3=features, 4=train)");
        // AddConfigEntry("last_step", "4", "The last step to perform");
        AddConfigEntry("align_in", "", "The input file for the alignments");
        AddConfigEntry("align_dev", "", "The dev file for the alignments  (for shift-reduce parser)");
        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("beam", "100", "The maximum beam size");
        AddConfigEntry("combine_blocks", "true", "Whether to combine alignments into blocks");
        AddConfigEntry("combine_brackets", "true", "Whether to combine alignments into brackets");
        AddConfigEntry("cost", "1e-3", "The rate at which to learn");
        AddConfigEntry("feature_profile", "seq=Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,BIAS%aT", "Which features to use ");
        AddConfigEntry("iterations", "15", "The number of iterations of training to perform.");
        AddConfigEntry("loss_profile", "chunk=1", "Which loss functions to use");
        AddConfigEntry("max_term", "5", "The maximum length of a terminal ");
        AddConfigEntry("max_state", "1", "The maximum numer of states in equality checking");
        AddConfigEntry("model_in", "", "Can read in a model and use it as the starting point for training");
        AddConfigEntry("model_out", "", "An output file for the model");
        AddConfigEntry("parse_in", "", "The input file for the parses");
        AddConfigEntry("shuffle", "false", "Whether to shuffle the input");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("source_dev", "", "The dev file for the source sentences (for shift-reduce parser)");
        AddConfigEntry("use_reverse", "false", "Whether to use reverse terminals ");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 1: {model,oracle}{loss,score} 2: detail for building and rescoring");
        AddConfigEntry("write_every_iter", "false", "Write the model out every time during training.");
        AddConfigEntry("update", "max", "Which update strategy to use {naive,early,max}");
    }
	
};

}

#endif
