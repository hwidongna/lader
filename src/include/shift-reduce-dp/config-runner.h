#ifndef CONFIG_RUNNER_H__
#define CONFIG_RUNNER_H__

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

namespace lader {

class ConfigRunner : public ConfigBase {

public:

	ConfigRunner() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 2;

        SetUsage(
"~~~ lader ~~~\n"
"  by Hwidong Na\n"
"\n"
"Reorders a text according to a discriminative model using shift-reduce algorithm.\n"
);

        AddConfigEntry("out_format", "string", "A comma seperated list of outputs (string/parse/order/score/flatten/action)");
        AddConfigEntry("model", "", "A model to be used in reordering");
        AddConfigEntry("beam", "20", "The maximum beam size");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 2: detail for building");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
    }
	
};

}

#endif
