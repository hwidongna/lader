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
#include <shift-reduce-dp/idpstate.h>
#include <boost/foreach.hpp>

using namespace boost;

namespace lader {

class IParserRanks : public Ranks{
public:
    IParserRanks() : Ranks() { }
    IParserRanks(const CombinedAlign & combined, CombinedAlign::NullHandler attach) :
			Ranks(combined), attach_(attach) { }
    // insert null-aligned target positions
    // note that the size of ranks/cal will change
    void Insert(CombinedAlign * cal){
    	int n = ranks_.size();
    	vector<CombinedAlign::Span> spans(cal->GetSpans());
    	sort(spans.begin(), spans.end());
    	// null span at the beginning
    	if (0 < spans[0].first){
    		CombinedAlign::Span nullspan(0, spans[0].first-1);
    		vector<CombinedAlign::Span> & calspans = cal->GetSpans();
			vector<CombinedAlign::Span>::iterator it;
			it = find(calspans.begin(), calspans.end(), spans[0]);
			int j = distance(calspans.begin(), it);
			// skip the tie ranks
			while (it != calspans.begin() && j > 0){
				if (ranks_[j-1] != ranks_[j])
					break;
				it--;
			}
			ranks_.insert(ranks_.begin()+j, -1);
			calspans.insert(it, nullspan);
    	}
    	// null span between words
    	for(int i = 1 ; i < n ; i++){
    		if (spans[i-1].second+1 < spans[i].first){
    			CombinedAlign::Span nullspan(spans[i-1].second+1, spans[i].first-1);
    			vector<CombinedAlign::Span> & calspans = cal->GetSpans();
    			vector<CombinedAlign::Span>::iterator it;
    			int j;
    			if (attach_ == CombinedAlign::ATTACH_NULL_LEFT){
    				it = find(calspans.begin(), calspans.end(), spans[i-1]);
    				it++; // insert the null span to the right of the source word
    				j = distance(calspans.begin(), it);
    				// skip the tie ranks
    				while (it != calspans.end() && j < ranks_.size()){
    					if (ranks_[j-1] != ranks_[j])
    						break;
    					it++;
    				}
    			}
    			else if (attach_ == CombinedAlign::ATTACH_NULL_RIGHT){
    				it = find(calspans.begin(), calspans.end(), spans[i]);
    				j = distance(calspans.begin(), it);
    				// skip the tie ranks
    				while (it != calspans.begin() && j > 0){
    					if (ranks_[j-1] != ranks_[j])
    						break;
    					it--;
    				}
    			}
    			ranks_.insert(ranks_.begin()+j, -1);
    			calspans.insert(it, nullspan);
    		}
    	}
    	// null span at the end, do we need this?
    }
    // Get reference action sequence from a combined word alignment
    // note that the size of ranks/cal will change
    // if you want to insert target position, call Insert before this method
    ActionVector GetReference(CombinedAlign * cal){
    	ActionVector result;
    	DPStateVector stateseq;
    	DPState * state = new IDPState();
    	stateseq.push_back(state);
    	int n = ranks_.size();
    	for (int step = 0 ; !state->Allow(DPState::IDLE, n) ; step++){
    		DPState * lstate = state->GetLeftState();
    		DPState::Action action;
    		if (state->Allow(DPState::DELETE_L, n) && IsDeleted(cal, lstate) && HasTie(lstate))
    			action = DPState::DELETE_L;
    		else if (state->Allow(DPState::DELETE_R, n) && IsDeleted(cal, state) && HasTie(lstate))
    			action = DPState::DELETE_R;
    		else if (state->Allow(DPState::STRAIGTH, n) && IsStraight(lstate, state) && !IsDeleted(cal, state))
    			action = DPState::STRAIGTH;
    		else if (state->Allow(DPState::INSERT_L, n) && ranks_[lstate->GetSrcL()] == Ranks::INSERT_L){
				action = DPState::INSERT_L;
				ranks_.erase(ranks_.begin() + state->GetSrcL());
				cal->GetSpans().erase(cal->GetSpans().begin() + state->GetSrcL());
				n--; // the size of rank and cal shrink
    		}
			else if (state->Allow(DPState::INSERT_R, n) && state->GetSrcREnd() < ranks_.size() && ranks_[state->GetSrcREnd()] == Ranks::INSERT_R){
				action = DPState::INSERT_R;
				ranks_.erase(ranks_.begin() + state->GetSrcREnd());
				cal->GetSpans().erase(cal->GetSpans().begin() + state->GetSrcREnd());
				n--; // the size of rank and cal shrink
			}
    		else if (state->Allow(DPState::INVERTED, n) && IsInverted(lstate, state) && !HasTie(state))
    			action = DPState::INVERTED;
    		else if (state->Allow(DPState::SHIFT, n))
    			action = DPState::SHIFT;
    		else
    			break;
    		result.push_back(action);
    		state->Take(action, stateseq, true); // only one item
    		if (state == stateseq.back()){
    			state->PrintTrace(cerr);
    			THROW_ERROR("Fail to get reference sequence" << endl);
    		}
    		state = stateseq.back();
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return result;
    }

    bool IsDeleted(const CombinedAlign * cal, DPState * state) const{
    	if (!cal)
    		return false;
    	return (state->GetSrcR() - state->GetSrcL()) == 1
    		&& cal->GetSpans()[state->GetSrcL()] == CombinedAlign::Span(-1,-1);
    }

private:
    CombinedAlign::NullHandler attach_;
};

}


#endif /* IPARSER_RANKS_H_ */
