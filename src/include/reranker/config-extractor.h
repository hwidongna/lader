#ifndef CONFIG_TRAINER_H__
#define CONFIG_TRAINER_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace reranker {

class ConfigExtractor : public lader::ConfigBase {

public:

    ConfigExtractor() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ train-reranker ~~~\n"
"  by Hwidong Na\n"
"\n"
"Extract features from k-best parses in bllip-parser format\n"
);

        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("kbest_in", "", "The input file for the kbest parses");
        AddConfigEntry("gold_in", "", "The gold-standard file for the source sentences");
        AddConfigEntry("max_swap", "0", "The maximum number of swap actions");
        AddConfigEntry("model_out", "", "An output file for the model");
        AddConfigEntry("model_in", "", "An input file for the model");
        AddConfigEntry("lexicalize-parent", "false", "Whether to lexicalize parent node");
        AddConfigEntry("lexicalize-child", "false", "Whether to lexicalize child node");
        AddConfigEntry("level", "2", "The number of level to use in word feature");
        AddConfigEntry("order", "2", "The order of n-gram to use in n-gram* feature");
        AddConfigEntry("threshold", "5", "The minimum number of feature counts for the reranker model");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 1: {model,oracle}{loss,score} 2: detail for building and rescoring");
    }
	
};

}

#endif
