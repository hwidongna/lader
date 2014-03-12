#ifndef CONFIG_ORACLE_H__
#define CONFIG_ORACLE_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

using namespace lader;
namespace reranker {

class ConfigOracle : public ConfigBase {

public:

    ConfigOracle() : ConfigBase() {
        minArgs_ = 2;
        maxArgs_ = 3;

        SetUsage(
"~~~ reranker-oracle ~~~\n"
"  by Hwidong Na\n"
"\n"
"Evaluates the reordering accuracy of k-best parsing result.\n"
"  Usage: evaluate-lader GOLDEN_ALIGNMENT SRC [TRG] < KBEST \n"
"\n"
"  GOLDEN_ALIGNMENT: [SRC_LEN]-[TRG_LEN] ||| f1-e1 f2-e2 f3-e3\n"
"  SRC: Sentence in the original source order\n"
"  TRG: Target sentence\n"
"  KBEST: trees from the k-best parser 1\n"
);
        AddConfigEntry("kbest_in", "", "The input file for the k-best parses");
        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("combine_blocks", "true", "Whether to attach the blocks together");
        AddConfigEntry("combine_brackets", "true", "Whether to combine alignments into brackets");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
        AddConfigEntry("max_swap", "0", "The maximum number of swap actions");
    }
	
};

}

#endif
