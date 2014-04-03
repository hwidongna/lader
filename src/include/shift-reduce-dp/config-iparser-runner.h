/*
 * config-iparser-runner.h
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#ifndef CONFIG_IPARSER_RUNNER_H_
#define CONFIG_IPARSER_RUNNER_H_

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigIParserRunner : public ConfigBase {

public:

	ConfigIParserRunner() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 2;

        SetUsage(
"~~~ lader ~~~\n"
"  by Hwidong Na\n"
"\n"
"Reorders a text according to a discriminative model using shift-reduce algorithm.\n"
);

        AddConfigEntry("out_format", "string", "A comma seperated list of outputs (string/parse/order/score/flatten/action)");
        AddConfigEntry("placeholder", "[*]", "An output symbol for the placeholder (default is [*])");
        AddConfigEntry("model", "", "A model to be used in reordering");
        AddConfigEntry("beam", "20", "The maximum beam size");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 2: detail for building");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
    }

};

}


#endif /* CONFIG_IPARSER_RUNNER_H_ */
