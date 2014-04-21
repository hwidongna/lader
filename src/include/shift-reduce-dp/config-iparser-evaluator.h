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
        minArgs_ = 3;
        maxArgs_ = 4;

        SetUsage(
"~~~ evaluate-iparser ~~~\n"
"  by Hwidong Na\n"
"\n"
"Evaluates the reordering accuracy of a particular reordering for a sentence.\n"
"  Usage: evaluate-iparser GOLDEN_ALIGNMENT DATA SRC [TRG]\n"
"\n"
"  GOLDEN_ALIGNMENT: [SRC_LEN]-[TRG_LEN] ||| f1-e1 f2-e2 f3-e3\n"
"  DATA: lader output \"a1 a2 ...\" where a1 is the reordered position of source word 1\n"
"  SRC: Sentence in the original source order\n"
"  TRG: Target sentence\n"
);

        AddConfigEntry("loss_profile", "levenshtein=1", "Which loss functions to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
        AddConfigEntry("placeholder", "<>", "An output symbol for the placeholder (default is <>)");
        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("attach_trg", "left", "Whether to attach target null alignments to the left or right");
        AddConfigEntry("combine_blocks", "true", "Whether to attach the blocks together");
        AddConfigEntry("combine_brackets", "true", "Whether to combine alignments into brackets");
        AddConfigEntry("insert", "true", "Whether to insert target words");
        AddConfigEntry("delete", "true", "Whether to delete source words");

    }
	
};

}

#endif
