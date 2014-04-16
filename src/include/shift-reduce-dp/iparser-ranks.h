/*
 * iparser-ranks.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef IPARSER_RANKS_H_
#define IPARSER_RANKS_H_

#include <vector>
#include <lader/ranks.h>

namespace lader {

class IParserRanks : public Ranks{
public:
    IParserRanks(): Ranks(), verbose_(false) { }
    IParserRanks(const CombinedAlign & combined, CombinedAlign::NullHandler attach_trg) :
			Ranks(combined), attach_trg_(attach_trg), verbose_(false) { }
    void SetVerbose(bool verbose) { verbose_ = verbose; }
    // insert null-aligned target positions
    // note that the size of ranks/cal will change
    void Insert(CombinedAlign * cal);
    // Get reference action sequence from a combined word alignment
    // note that the size of ranks/cal will change
    // if you want to insert target position, call Insert before this method
    ActionVector GetReference(CombinedAlign * cal);

    bool IsDeleted(const CombinedAlign * cal, DPState * state) const;

private:
    CombinedAlign::NullHandler attach_trg_; // Where to attach nulls
    bool verbose_;
};

}


#endif /* IPARSER_RANKS_H_ */
