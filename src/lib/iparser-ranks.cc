/*
 * iparser-ranks.cc
 *
 *  Created on: Apr 16, 2014
 *      Author: leona
 */
#include <shift-reduce-dp/iparser-ranks.h>
#include <shift-reduce-dp/idpstate.h>
#include <boost/foreach.hpp>
#include <iostream>
using namespace boost;
using namespace std;

namespace lader{

// insert null-aligned target positions
// note that the size of ranks/cal will be increased
void IParserRanks::Insert(CombinedAlign * cal){
	int n = ranks_.size();
	vector<CombinedAlign::Span> spans(cal->GetSpans());
	sort(spans.begin(), spans.end());
	if (verbose_){
		BOOST_FOREACH(CombinedAlign::Span s, spans)
			cerr << "[" << s.first << "," << s.second << "] ";
		cerr << endl;
	}
	int k = 0 ;
	while(spans[k] == CombinedAlign::Span(-1,-1) && k < spans.size())
		 k++; // skip the null-aligned source positions
	if (verbose_){
		cerr << "position " << k << ":";
		BOOST_FOREACH(int r, ranks_){
			cerr << " " << r;
		}
		cerr << endl;
		BOOST_FOREACH(CombinedAlign::Span s, cal->GetSpans())
			cerr << "[" << s.first << "," << s.second << "] ";
		cerr << endl;
	}
	// null span at the beginning
	if (0 < spans[k].first){
		CombinedAlign::Span nullspan(0, spans[k].first-1);
		vector<CombinedAlign::Span> & calspans = cal->GetSpans();
		vector<CombinedAlign::Span>::iterator it;
		if (verbose_)
			cerr << "find [" << spans[k].first << "," << spans[k].second << "] " << endl;
		it = find(calspans.begin(), calspans.end(), spans[k]);
		int j = distance(calspans.begin(), it);
		// skip the tie ranks
		while (it != calspans.begin() && j > 0){
			if (ranks_[j-1] != ranks_[j])
				break;
			it--; j--;
		}
		ranks_.insert(ranks_.begin()+j, Ranks::INSERT_L);
		calspans.insert(it, nullspan);
	}
	// null span between words
	for(int i = k+1 ; i < n ; i++){
		if (spans[i-1].second+1 < spans[i].first){
			if (verbose_){
				cerr << "position " << i << ":";
				BOOST_FOREACH(int r, ranks_){
					cerr << " " << r;
				}
				cerr << endl;
				BOOST_FOREACH(CombinedAlign::Span s, cal->GetSpans())
				cerr << "[" << s.first << "," << s.second << "] ";
				cerr << endl;
			}
			CombinedAlign::Span nullspan(spans[i-1].second+1, spans[i].first-1);
			vector<CombinedAlign::Span> & calspans = cal->GetSpans();
			vector<CombinedAlign::Span>::iterator it;
			int j;
			if (attach_trg_ == CombinedAlign::ATTACH_NULL_LEFT){
				it = find(calspans.begin(), calspans.end(), spans[i-1]);
				it++; // insert the null span to the right of the source word
				j = distance(calspans.begin(), it);
				// skip the tie ranks
				while (it != calspans.end() && j < ranks_.size()){
					if (ranks_[j-1] != ranks_[j])
						break;
					it++; j++;
				}
				ranks_.insert(ranks_.begin()+j, Ranks::INSERT_R);
			}
			else if (attach_trg_ == CombinedAlign::ATTACH_NULL_RIGHT){
				it = find(calspans.begin(), calspans.end(), spans[i]);
				j = distance(calspans.begin(), it);
				// skip the tie ranks
				while (it != calspans.begin() && j > 0){
					if (ranks_[j-1] != ranks_[j])
						break;
					it--; j--;
				}
				ranks_.insert(ranks_.begin()+j, Ranks::INSERT_L);
			}
			calspans.insert(it, nullspan);
		}
	}
	if (verbose_){
		cerr << "position " << n << ":";
		BOOST_FOREACH(int r, ranks_){
			cerr << " " << r;
		}
		cerr << endl;
		BOOST_FOREACH(CombinedAlign::Span s, cal->GetSpans())
			cerr << "[" << s.first << "," << s.second << "] ";
		cerr << endl;
	}
	if (ranks_.size() != cal->GetSpans().size())
		THROW_ERROR("Ranks.size() != calspan.size(): "
				<< ranks_.size() << " != " << cal->GetSpans().size() << endl);
	// null span at the end, do we need this?
}
// Get reference action sequence from a combined word alignment
// note that the size of ranks/cal will be decreased to the original sentence length
// if you want to insert target position, call Insert before this method
ActionVector IParserRanks::GetReference(CombinedAlign * cal){
	ActionVector result;
	DPStateVector stateseq;
	IDPState * state = new IDPState();
	stateseq.push_back(state);
	int n = ranks_.size();
	for (int step = 0 ; !state->Allow(DPState::IDLE, n) ; step++){
		DPState * lstate = state->GetLeftState();
		if (verbose_){
			if (lstate){
				lstate->Print(cerr); cerr << "\t";
			}
			state->Print(cerr); cerr << endl;
			BOOST_FOREACH(int r, ranks_){
				cerr << " " << r;
			}
			cerr << endl;
			BOOST_FOREACH(CombinedAlign::Span s, cal->GetSpans())
				cerr << "[" << s.first << "," << s.second << "] ";
			cerr << endl;
		}
		DPState::Action action;
		if (state->Allow(DPState::INSERT_L, n) && !IsDeleted(cal, state) && lstate->GetAction() != DPState::INIT && ranks_[lstate->GetSrcL()] == Ranks::INSERT_L){
			action = DPState::INSERT_L;
			ranks_.erase(ranks_.begin() + state->GetSrcL()-1);
			cal->GetSpans().erase(cal->GetSpans().begin() + state->GetSrcL()-1);
			n--; // the size of rank and cal shrink
			result.erase(result.begin()+lstate->GetStep()-1); // rollback the shift action
			stateseq.erase(stateseq.begin()+lstate->GetStep());
			state->Rollback(lstate);
			delete lstate;
		}
		else if (state->Allow(DPState::INSERT_R, n) && !IsDeleted(cal, state) && state->GetSrcREnd() < ranks_.size() && ranks_[state->GetSrcREnd()] == Ranks::INSERT_R){
			action = DPState::INSERT_R;
			ranks_.erase(ranks_.begin() + state->GetSrcREnd());
			cal->GetSpans().erase(cal->GetSpans().begin() + state->GetSrcREnd());
			n--; // the size of rank and cal shrink
		}
		else if (state->Allow(DPState::DELETE_L, n) && IsDeleted(cal, lstate) && !IsDeleted(cal, state) && HasTie(lstate))
			action = DPState::DELETE_L;
		else if (state->Allow(DPState::DELETE_R, n) && !IsDeleted(cal, lstate) && IsDeleted(cal, state) && HasTie(lstate))
			action = DPState::DELETE_R;
		else if (state->Allow(DPState::STRAIGTH, n) && !IsDeleted(cal, lstate) && !IsDeleted(cal, state) && IsStraight(lstate, state))
			action = DPState::STRAIGTH;
		else if (state->Allow(DPState::INVERTED, n) && !IsDeleted(cal, lstate) && !IsDeleted(cal, state) && IsInverted(lstate, state) && !HasTie(state))
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
		state = dynamic_cast<IDPState*>(stateseq.back());
	}
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
	if (ranks_.size() != cal->GetSpans().size())
		THROW_ERROR("Ranks.size() != calspan.size(): "
				<< ranks_.size() << " != " << cal->GetSpans().size() << endl);
	return result;
}

bool IParserRanks::IsDeleted(const CombinedAlign * cal, DPState * state) const{
	if (!cal)
		return false;
	return (state->GetSrcR() - state->GetSrcL()) == 1 // delete once a word
		&& cal->GetSpans()[state->GetSrcL()] == CombinedAlign::Span(-1,-1);
}

}
