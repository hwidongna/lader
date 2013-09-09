/*
 * config-oracle.h
 *
 *  Created on: Sep 5, 2013
 *      Author: leona
 */

#ifndef CONFIG_REORDERER_H_
#define CONFIG_REORDERER_H_

#include "config-base.h"

namespace lader {

class ConfigOracle: public ConfigBase {
public:
	ConfigOracle(){
        minArgs_ = 2;
        maxArgs_ = 2;

        SetUsage(
"~~~ reorderer ~~~\n"
"  by Hwidong Na\n"
"\n"
"Obtain the oracle reordering for a sentence.\n"
"  Usage: evaluate-lader GOLDEN_ALIGNMENT SRC\n"
"\n"
"  GOLDEN_ALIGNMENT: [SRC_LEN]-[TRG_LEN] ||| f1-e1 f2-e2 f3-e3\n"
"  SRC: Sentence in the original source order\n"
);

        AddConfigEntry("attach_null", "right", "Whether to attach null alignments to the left or right");
        AddConfigEntry("combine_blocks", "true", "Whether to attach the blocks together");
	}
	virtual ~ConfigOracle() {}
};

} /* namespace lader */
#endif /* CONFIG_REORDERER_H_ */