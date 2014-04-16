/*
 * config-iparser-gold.h
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#ifndef CONFIG_IPARSER_GOLD_H_
#define CONFIG_IPARSER_GOLD_H_

#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <lader/util.h>
#include <lader/config-base.h>

using namespace lader;

namespace reranker{

class ConfigIParserGold : public ConfigBase {

public:

    ConfigIParserGold() : ConfigBase() {
        minArgs_ = 0;
        maxArgs_ = 0;

        SetUsage(
"~~~ gold-tree ~~~\n"
"  by Hwidong Na\n"
"\n"
"Produce the gold-standard ITG tree from an word-aligned corpus\n"
);
        AddConfigEntry("out_format", "string", "A comma seperated list of outputs (string/parse/order/flatten/action)");
        AddConfigEntry("placeholder", "<>", "An output symbol for the placeholder (default is <>)");
        AddConfigEntry("align_in", "", "The input file for the alignments");
        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("attach_trg", "left", "Whether to attach null alignments to the left or right");
        AddConfigEntry("combine_blocks", "true", "Whether to combine alignments into blocks");
        AddConfigEntry("combine_brackets", "true", "Whether to combine alignments into brackets");
        AddConfigEntry("feature_profile", "seq=X", "Which features to use (meaningless) ");
        AddConfigEntry("source_in", "", "The input file for the source sentences");
        AddConfigEntry("align_in", "", "The input file for the alignments");
        AddConfigEntry("verbose", "0", "The level of debugging output to print");
        AddConfigEntry("threads", "1", "The number of threads to use");
    }

};

}


#endif /* CONFIG_IPARSER_GOLD_H_ */
