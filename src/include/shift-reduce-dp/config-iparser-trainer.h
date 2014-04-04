/*
 * config-iparser-trainer.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef CONFIG_IPARSER_TRAINER_H_
#define CONFIG_IPARSER_TRAINER_H_

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigIParserTrainer : public ConfigBase {

public:

    ConfigIParserTrainer() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ train-iparser ~~~\n"
"  by Hwidong Na\n"
"\n"
"Trains a discriminative model for machine translation using shift-reduce algorithm.\n"
);

        AddConfigEntry("beam", "100", "The maximum beam size");
        AddConfigEntry("cost", "1e-3", "The rate at which to learn");
        AddConfigEntry("feature_profile", "seq=Q0%q0%aT,LL0%s0L%aT,RR0%s0R%aT,LR0%l0R%aT,RL0%r0L%aT,O0%s0L%s0R%aT,I0%l0R%r0L%aT,BIAS%aT", "Which features to use ");
        AddConfigEntry("iterations", "15", "The number of iterations of training to perform.");
        AddConfigEntry("loss_profile", "levenshtein=1", "Which loss functions to use");
        AddConfigEntry("max_state", "1", "The maximum numer of states in equality checking");
        AddConfigEntry("max_ins", "1", "The maximum numer of insert actions");
        AddConfigEntry("max_del", "1", "The maximum numer of delete actions");
        AddConfigEntry("model_in", "", "Can read in a model and use it as the starting point for training");
        AddConfigEntry("model_out", "", "An output file for the model");
        AddConfigEntry("shuffle", "false", "Whether to shuffle the input");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("source_gold", "", "The gold action sequence file for the source sentences");
        AddConfigEntry("target_gold", "", "The gold action sequence file for the target sentences");
        AddConfigEntry("source_dev", "", "The dev file for the source sentences");
        AddConfigEntry("source_dev_gold", "", "The dev gold action sequence file for the source sentences");
        AddConfigEntry("target_dev_gold", "", "The dev gold action sequence file for the target sentences");
        AddConfigEntry("use_reverse", "false", "Whether to use reverse terminals ");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
        AddConfigEntry("write_every_iter", "false", "Write the model out every time during training.");
        AddConfigEntry("update", "max", "Which update strategy to use {naive,early,max}");
    }

};

}


#endif /* CONFIG_IPARSER_TRAINER_H_ */
