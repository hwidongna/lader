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
    IParserRanks() : Ranks() { }
    // Turn a combined alignment into its corresponding ranks
    IParserRanks(const CombinedAlign & combined) : Ranks(combined) { }
    // This method gets monolingual reference action sequences without INSERT_[LR]
    // The bilingual reference sequence with INSERT_[LR] is obtained from
    // the intersection of bidirectional GetReference
    ActionVector GetReference(const CombinedAlign * cal) const{
    	ActionVector reference;
    	DPStateVector stateseq;
    	DPState * state = new IDPState();
    	stateseq.push_back(state);
    	int n = ranks_.size();
    	for (int step = 0 ; !state->Allow(DPState::IDLE, n) ; step++){
    		DPState * leftstate = state->GetLeftState();
    		DPState::Action action;
    		if (state->Allow(DPState::DELETE_L, n) && IsDeleted(cal, leftstate) && HasTie(leftstate))
    			action = DPState::DELETE_L;
    		else if (state->Allow(DPState::DELETE_R, n) && IsDeleted(cal, state) && HasTie(leftstate))
    			action = DPState::DELETE_R;
    		else if (state->Allow(DPState::STRAIGTH, n) && IsStraight(leftstate, state))
    			action = DPState::STRAIGTH;
    		else if (state->Allow(DPState::INVERTED, n) && IsInverted(leftstate, state) && !HasTie(state))
    			action = DPState::INVERTED;
    		else if (state->Allow(DPState::SHIFT, n))
    			action = DPState::SHIFT;
    		else
    			break;
    		reference.push_back(action);
    		state->Take(action, stateseq, true); // only one item
    		if (state == stateseq.back()){
    			state->PrintTrace(cerr);
    			THROW_ERROR("Fail to get reference sequence" << endl);
    		}
    		state = stateseq.back();
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return reference;
    }
};

}


#endif /* IPARSER_RANKS_H_ */
