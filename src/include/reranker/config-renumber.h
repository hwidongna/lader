/*
 * config-renumber.h
 *
 *  Created on: Feb 27, 2014
 *      Author: leona
 */

#ifndef CONFIG_RENUMBER_H_
#define CONFIG_RENUMBER_H_

#include <lader/config-base.h>

namespace reranker {

class ConfigRenumber : public lader::ConfigBase {

public:

    ConfigRenumber() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ train-reranker ~~~\n"
"  by Hwidong Na\n"
"\n"
"Renumber the feature ids and discard features with occurence below threshold\n"
);

        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("model_out", "", "An output file for the model");
        AddConfigEntry("model_in", "", "An input file for the model");
        AddConfigEntry("threshold", "5", "The minimum number of feature counts for the reranker model");
    }

};

}


#endif /* CONFIG_RENUMBER_H_ */
