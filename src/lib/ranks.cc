#include <lader/ranks.h>
#include <lader/util.h>
#include <boost/foreach.hpp>
#include <map>
#include <iostream>
#include <shift-reduce-dp/ddpstate.h>

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

std::vector<DPState::Action> Ranks::GetReference(int m) const{
	std::vector<DPState::Action> reference;
	DPStateVector stateseq;
	DPState * state = new DDPState();
	stateseq.push_back(state);
	int n = ranks_.size();
	for (int step = 1 ; step < 2*(n+m) ; step++){
		DPState * leftstate = state->GetLeftState();
		DPState::Action action;
		if (state->Allow(DPState::STRAIGTH, n) && IsStraight(leftstate, state))
			action = DPState::STRAIGTH;
		else if (state->Allow(DPState::INVERTED, n) && IsInverted(leftstate, state) && !HasTie(state))
			action = DPState::INVERTED;
		else if (m > 0 && state->Allow(DPState::SWAP, n) && !HasTie(state) && HasContinuous(state))
			action = DPState::SWAP;
		else if (state->Allow(DPState::IDLE, n))
			action = DPState::IDLE;
		else if (state->Allow(DPState::SHIFT, n))
			action = DPState::SHIFT;
		else // fail to get reference
			break;
		reference.push_back(action);
		state->Take(action, stateseq, true); // only one item
		state = stateseq.back();
	}
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
	return reference;
}

bool Ranks::IsStraight(DPState * leftstate, DPState * state) const{
	if (!leftstate)
		return false;
	return Ranks::IsContiguous(ranks_[leftstate->GetTrgR()-1], ranks_[state->GetTrgL()]);
}

bool Ranks::IsInverted(DPState * leftstate, DPState * state) const{
	if (!leftstate)
		return false;
	return 	Ranks::IsStepOneUp(ranks_[state->GetTrgR()-1], ranks_[leftstate->GetTrgL()]); // strictly step-one up
}

bool Ranks::HasTie(DPState * state) const{
	return 	state->GetSrcREnd() < ranks_.size()	// state->GetSrcREnd() will be the next buffer item
			&& ranks_[state->GetTrgR()-1] == ranks_[state->GetSrcREnd()];// avoid tie ranks in buffer
}


bool Ranks::HasContinuous(DPState * state) const {
	DPState * leftstate = state->GetLeftState();
	while (leftstate){
		if (IsStraight(leftstate, state) || IsInverted(leftstate, state))
			return true;
		leftstate = leftstate->GetLeftState();
	}
	return false;
}
