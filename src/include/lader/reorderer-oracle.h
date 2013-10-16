/*
 * reorderer-oracle.h
 *
 *  Created on: Oct 16, 2013
 *      Author: leona
 */
#ifndef REORDERER_ORACLE_H_
#define REORDERER_ORACLE_H_


#include <lader/config-oracle.h>
#include <lader/combined-alignment.h>

namespace lader {

class ReordererOracle {
public:
	ReordererOracle(): attach_(CombinedAlign::ATTACH_NULL_LEFT),
    combine_(CombinedAlign::COMBINE_BLOCKS),
    bracket_(CombinedAlign::ALIGN_BRACKET_SPANS)
     { }
	virtual ~ReordererOracle() {}

	void Oracle(const ConfigOracle & config);

private:
    CombinedAlign::NullHandler attach_;
    CombinedAlign::BlockHandler combine_;
    CombinedAlign::BracketHandler bracket_;
};

} /* namespace lader */

#endif /* REORDERER_ORACLE_H_ */
