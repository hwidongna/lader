#ifndef CONFIG_EVALUATOR_H__
#define CONFIG_EVALUATOR_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigEvaluator : public ConfigBase {

public:

    ConfigEvaluator() : ConfigBase() {
        minArgs_ = 3;
        maxArgs_ = 4;

        SetUsage(
"~~~ evaluate-lader ~~~\n"
"  by Graham Neubig\n"
"\n"
"Evaluates the reordering accuracy of a particluar reordering for a sentence.\n"
"  Usage: evaluate-lader GOLDEN_ALIGNMENT HYP SRC [TRG]\n"
"\n"
"  GOLDEN_ALIGNMENT: [SRC_LEN]-[TRG_LEN] ||| f_j-e_i ... \n"
"  HYP: a1 ... where a1 is the reordered position of source word 1\n"
"  SRC: Sentence in the original source order\n"
"  TRG: Target sentence in the original target order\n"
);

        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("combine_blocks", "true", "Whether to attach the blocks together");
        AddConfigEntry("combine_brackets", "true", "Whether to combine alignments into brackets");
    }
	
};

}

#endif
