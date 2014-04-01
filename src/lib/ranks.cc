#include <lader/ranks.h>
#include <lader/util.h>
#include <boost/foreach.hpp>
#include <map>
#include <iostream>
#include <shift-reduce-dp/ddpstate.h>
#include <shift-reduce-dp/idpstate.h>
#include <climits>
#include <set>
using namespace std;
using namespace lader;

// Find the ranks of each 
Ranks::Ranks(const CombinedAlign & combined) {
    // Sort the combined alignments in rank order (allowing ties)
    typedef pair<pair<double,double>, vector<int> > RankPair;
    typedef map<pair<double,double>, vector<int>, AlignmentIsLesser> RankMap;
    RankMap rank_map;
    for(int i = 0; i < (int)combined.GetSrcLen(); i++) {
        RankMap::iterator it = rank_map.find(combined[i]);
        if(it == rank_map.end()) {
            rank_map.insert(MakePair(combined[i], vector<int>(1,i)));
        } else {
            it->second.push_back(i);
        }
    }
    // Build the ranks
    ranks_.resize(combined.GetSrcLen());
    max_rank_ = -1;
    BOOST_FOREACH(RankPair rp, rank_map) {
        ++max_rank_;
        BOOST_FOREACH(int i, rp.second)
            ranks_[i] = max_rank_;
    }
}

void Ranks::SetRanks(const std::vector<int> & order) {
	if(order.size() != ranks_.size())
		THROW_ERROR("Vector sizes in Reorder don't match: " << order.size() << " != " << ranks_.size() << endl)
	// overwrite the new order as ranks
	ranks_ = order;
	max_rank_ = order.size()-1;
}

// This method gets monolingual reference action sequences without INSERT_[LR]
// The bilingual reference sequence with INSERT_[LR] is obtained from
// the intersection of bidirectional GetReference
ActionVector Ranks::GetReference(CombinedAlign * cal) const{
	ActionVector reference;
	DPStateVector stateseq;
	DPState * state;
	if (cal)
		state = new IDPState();
	else
		state = new DDPState();
	stateseq.push_back(state);
	int n = ranks_.size();
	for (int step = 0 ; !state->Allow(DPState::IDLE, n) ; step++){
		DPState * leftstate = state->GetLeftState();
		DPState::Action action;
		if (state->Allow(DPState::DELETE_L, n) && IsDeleted(cal, leftstate))
			action = DPState::DELETE_L;
		else if (state->Allow(DPState::DELETE_R, n) && IsDeleted(cal, state))
			action = DPState::DELETE_R;
		else if (state->Allow(DPState::STRAIGTH, n) && IsStraight(leftstate, state))
			action = DPState::STRAIGTH;
		else if (state->Allow(DPState::INVERTED, n) && IsInverted(leftstate, state) && !HasTie(state))
			action = DPState::INVERTED;
		else if (state->Allow(DPState::SWAP, n) && IsSwap(state))
			action = DPState::SWAP;
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

bool Ranks::IsDeleted(CombinedAlign * cal, DPState * state) const{
	if (!cal)
		return false;
	const std::vector<std::pair<double,double> > & spans = cal->GetSpans();
	return spans[state->GetSrcL()].first == -1;
}
bool Ranks::IsStraight(DPState * lstate, DPState * state) const{
	if (!lstate)
		return false;
	int l = min(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	int r = max(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	int ll = min(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
	int lr = max(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
	return Ranks::IsContiguous(lr, l);
}

bool Ranks::IsInverted(DPState * lstate, DPState * state) const{
	if (!lstate)
		return false;
	int l = min(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	int r = max(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	int ll = min(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
	int lr = max(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
	return 	Ranks::IsStepOneUp(r, ll); // strictly step-one up
}

bool Ranks::HasTie(DPState * state) const{
	int r = max(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	return 	state->GetSrcREnd() < ranks_.size()	// state->GetSrcREnd() will be the next buffer item
			&& r == ranks_[state->GetSrcREnd()];// avoid tie ranks in buffer
}

// check whether a reducible item exists in stack, not in buffer
bool Ranks::IsSwap(DPState * state) const {
	int l = min(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	int r = max(ranks_[state->GetTrgL()], ranks_[state->GetTrgR()-1]);
	// check stack
	DPState * lstate = state->GetLeftState();
	bool flag = false;
	while (lstate){
		int ll = min(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
		int lr = max(ranks_[lstate->GetTrgL()], ranks_[lstate->GetTrgR()-1]);
		if (Ranks::IsContiguous(lr, l) || Ranks::IsContiguous(r, ll)){
			flag = true; // found the reducible item in stack
			break;
		}
		lstate = lstate->GetLeftState();
	}
	// check buffer
	if (state->GetSrcREnd() < ranks_.size()){
		vector<int> seq;
		int n = ranks_.size();
		Span span = state->GetSrcSpan();
		DPStateVector stateseq;
		state->Take(DPState::SHIFT, stateseq);
		span.second++;
		while (!state->Allow(DPState::IDLE, n)){
			DPState * state = stateseq.back();
			if (state->GetSrcSpan() == span){
				flag = false; // found the reducible item in buffer
				break;
			}
			DPState * leftstate = state->GetLeftState();
			DPState::Action action;
			if (state->Allow(DPState::STRAIGTH, n) && IsStraight(leftstate, state))
				action = DPState::STRAIGTH;
			else if (state->Allow(DPState::INVERTED, n) && IsInverted(leftstate, state) && !HasTie(state))
				action = DPState::INVERTED;
			else if (state->Allow(DPState::SHIFT, n)){
				action = DPState::SHIFT;
				span.second++;
			}
			else
				break;
			state->Take(action, stateseq); // only one item
		}
		BOOST_FOREACH(DPState * state, stateseq)
			delete state;
	}
	return flag;
}

// check whether a reducible item exists in stack
bool Ranks::HasContinuous(DPState * state) const {
	DPState * leftstate = state->GetLeftState();
	while (leftstate){
		if (IsStraight(leftstate, state) || IsInverted(leftstate, state))
			return true;
		leftstate = leftstate->GetLeftState();
	}
	return false;
}
