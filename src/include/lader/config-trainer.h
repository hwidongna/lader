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
"~~~ train-lader ~~~\n"
"  by Graham Neubig\n"
"  by Hwidong Na for discontinuous version\n"
"\n"
"Trains a model discriminative model for machine translation reordering.\n"
);

        // AddConfigEntry("first_step", "1", "The first step (1=hypergraph, 2=loss, 3=features, 4=train)");
        // AddConfigEntry("last_step", "4", "The last step to perform");
        AddConfigEntry("align_in", "", "The input file for the alignments");
        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("beam", "100", "The maximum beam size");
        AddConfigEntry("gap-size", "1", "The gap size for discontinuous hyper graph");
        AddConfigEntry("full_fledged", "false", "Enable discontinuous hyper graph full-fledged combinations");
        AddConfigEntry("mp", "false", "Monotone at punctuation");
        AddConfigEntry("cube_growing", "true", "Use Cube Growing for construction of hyper graph");
        AddConfigEntry("combine_blocks", "true", "Whether to combine alignments into blocks");
        AddConfigEntry("cost", "1e-3", "The rate at which to learn");
        AddConfigEntry("feature_profile", "seq=LL%SL%ET,RR%SR%ET,LR%LR%ET,RL%RL%ET,O%SL%SR%ET,I%LR%RL%ET,CL%CL%ET,B%SB%ET,A%SA%ET,N%SN%ET,BIAS%ET", "Which features to use ");
        AddConfigEntry("non_local_feature_profile", "", "Which non-local features to use ");
        AddConfigEntry("iterations", "100", "The number of iterations of training to perform.");
        AddConfigEntry("samples", "500", "The number of samples of training to perform.");
        AddConfigEntry("loss_profile", "chunk=1", "Which loss functions to use");
        AddConfigEntry("loss_augmented_inference", "true", "Whether to use loss-augmented inference");
        AddConfigEntry("max_term", "5", "The maximum length of a terminal ");
        AddConfigEntry("model_out", "", "An output file for the model");
        AddConfigEntry("parse_in", "", "The input file for the parses");
        AddConfigEntry("save_features", "false", "Save the features in memory after generating them once (more memory, more speed)");
        AddConfigEntry("shuffle", "true", "Whether to shuffle the input");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("use_reverse", "false", "Whether to use reverse terminals ");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 1: {model,oracle}{loss,score} 2: detail for building and rescoring");
        AddConfigEntry("write_every_iter", "true", "Write the model out every time during training.");

    }
	
};

}

#endif
