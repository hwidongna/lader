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
"  by Graham Neubig\n"
"  by Hwidong Na for discontinuous version\n"
"\n"
"Reorders a text according to a discriminative model.\n"
);

        AddConfigEntry("out_format", "string", "A comma seperated list of outputs (string/parse/order)");
        AddConfigEntry("model", "", "A model to be used in reordering");
        AddConfigEntry("beam", "1", "The maximum beam size");
        AddConfigEntry("pop_limit", "10000", "The maximum pop count (for cube growing)");
        AddConfigEntry("gap-size", "1", "The gap size for discontinuous hyper graph");
        AddConfigEntry("max-seq", "2", "The maximum number of sequential discontinuous hypothesis");
        AddConfigEntry("full_fledged", "false", "Enable discontinuous hyper graph full-fledged combinations");
        AddConfigEntry("backward_compatibility", "false", "Support model trained from previous version without non-local feature templates");
        AddConfigEntry("mp", "false", "Monotone at punctuation");
        AddConfigEntry("cube_growing", "true", "Use Cube Growing for construction of hyper graph");
        AddConfigEntry("bigram", "", "The bigram file for reordered language model");
        AddConfigEntry("threads", "1", "The number of threads to use");
        AddConfigEntry("verbose", "0", "The level of debugging output to print 2: detail for building");
        AddConfigEntry("source_in", "", "The input file for the source sentences");

    }
	
};

}

#endif
