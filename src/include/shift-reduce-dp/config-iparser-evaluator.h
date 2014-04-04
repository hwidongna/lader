#ifndef CONFIG_EVALUATOR_H__
#define CONFIG_EVALUATOR_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigIParserEvaluator : public ConfigBase {

public:

    ConfigIParserEvaluator() : ConfigBase() {
        minArgs_ = 2;
        maxArgs_ = 3;

        SetUsage(
"~~~ evaluate-lader ~~~\n"
"  by Graham Neubig\n"
"\n"
"Evaluates the reordering accuracy of a particular reordering for a sentence.\n"
"  Usage: evaluate-lader DATA SRC [TRG]\n"
"\n"
"  DATA: lader output \"a1 a2 ...\" where a1 is the reordered position of source word 1\n"
"  SRC: Sentence in the original source order\n"
"  TRG: Target sentence\n"
);

        AddConfigEntry("source_gold", "", "The gold action sequence file for the source sentences");
        AddConfigEntry("target_gold", "", "The gold action sequence file for the target sentences");
        AddConfigEntry("loss_profile", "levenshtein=1", "Which loss functions to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
        AddConfigEntry("placeholder", "[*]", "An output symbol for the placeholder (default is [*])");
    }
	
};

}

#endif
