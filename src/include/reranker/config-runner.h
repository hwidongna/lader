/*
 * config-runner.h
 *
 *  Created on: Feb 27, 2014
 *      Author: leona
 */

#ifndef CONFIG_RUNNER_H_
#define CONFIG_RUNNER_H_

#include <lader/config-base.h>

namespace reranker {

class ConfigRunner : public lader::ConfigBase {

public:

    ConfigRunner() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ train-reranker ~~~\n"
"  by Hwidong Na\n"
"\n"
"Reranker for k-best output using reranking model and weights\n"
);
        AddConfigEntry("out_format", "string", "A comma seperated list of outputs (string/parse/order/scoreflatten/action)");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("model_in", "", "An input file for the model");
        AddConfigEntry("weights", "", "An input file for the model weights");
        AddConfigEntry("lexicalize-parent", "false", "Whether to lexicalize parent node");
        AddConfigEntry("lexicalize-child", "false", "Whether to lexicalize child node");
        AddConfigEntry("level", "2", "The number of level to use in word feature");
        AddConfigEntry("order", "2", "The order of n-gram to use in n-gram* feature");
        AddConfigEntry("trees", "0", "The number of trees to use, (default is 0, which means use all possible trees)");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
    }

};

}


#endif /* CONFIG_RUNNER_H_ */
