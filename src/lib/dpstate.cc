/*
 * dpstate.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/dpstate.h"
#include <cstdlib>
#include <boost/foreach.hpp>
using namespace std;

namespace lader {

// initial state
DPState::DPState() {
	score_ = 0; inside_ = 0; shiftcost_ = 0;
	step_ = 0;
	i_ = 0; j_ = 0;
	k_ = 0; l_ = 0;
	rank_ = -1;
	action_ = DPState::INIT;
	gold_ = true;
}

// new state
// set k_ and l_ latter
DPState::DPState(int step, int i, int j, Action action) {
	score_ = 0, inside_ = 0, shiftcost_ = 0;
	step_ = step;
	i_ = i, j_ = j;
	rank_ = -1;
	action_ = action;
	gold_ = false;
}

DPState::~DPState() {
}

void DPState::MergeWith(DPState * other){
	if (action_ == DPState::SHIFT)
		leftptrs_.push_back(other->leftptrs_[0]);
	else // reduce
		if (keep_alternatives_)
			backptrs_.push_back(other->backptrs_[0]);
}
vector<DPState*> DPState::Take(Action action, bool actiongold){
	double 	actioncost = 0;
	vector<DPState*> result;
	// TODO: get action cost
//	if (model_ != NULL)
//		actioncost =
	if (action == DPState::SHIFT){
		DPState * next = Shift();
		next->inside_ = 0;
		next->score_ = score_ + actioncost;
		shiftcost_ = actioncost;
		next->gold_ = gold_ && actiongold;
		next->leftptrs_.push_back(this);
		BackPtr back;
		back.action = action;
		back.cost = 0;
		back.leftstate = NULL;
		back.istate = NULL;
		next->backptrs_.push_back(back);
		result.push_back(next); // copy, right?
	}
	else{ // reduce
		BOOST_FOREACH(DPState * leftstate, leftptrs_){
			DPState * next = Reduce(leftstate, action);
			next->inside_ = leftstate->inside_ + inside_ + leftstate->shiftcost_ + actioncost;
			next->score_ = leftstate->score_ + inside_ + leftstate->shiftcost_ + actioncost;
			next->gold_ = leftstate->gold_ && gold_ && actiongold;
			next->leftptrs_ = leftstate->leftptrs_;
			BackPtr back;
			back.action = action;
			back.cost = leftstate->shiftcost_ + actioncost;
			back.leftstate = leftstate;
			back.istate = this;
			next->backptrs_.push_back(back);
			result.push_back(next);
		}
	}
	return result;
}


DPState * DPState::Shift(){
	DPState * next = new DPState(step_+1, j_, j_+1, DPState::SHIFT);
	next->k_ = j_; next->l_ = j_+1;
	return next;
}
DPState * DPState::Reduce(DPState * leftstate, Action action){
	DPState * next = new DPState(step_+1, leftstate->i_, j_, action);
	if (action == STRAIGTH){
		next->k_ = leftstate->k_;		next->l_ = l_;
	}
	else if (action == INVERTED){
		next->k_ = k_;		next->l_ = leftstate->l_;
	}
	return next;
}

bool DPState::Allow(const Action & action, const int n){
	if (action == DPState::SHIFT)
		return j_ < n;
	DPState * leftstate = GetLeftState();
	return leftstate && leftstate->action_ != INIT
			&& (leftstate->j_ == i_ || j_ == leftstate->i_);
}

void DPState::InsideActions(vector<Action> & result){
	if (!backptrs_.empty()){
		if (action_ == DPState::STRAIGTH || action_ == DPState::INVERTED){ // reduce
			BackPtr & back = backptrs_[0];
			back.leftstate->AllActions(result);
			back.istate->AllActions(result);
		}
		result.insert(result.begin(), action_);
	}
}

void DPState::AllActions(vector<Action> & result){
	DPState * item = this;
	while (!item->leftptrs_.empty()){
		item->InsideActions(result);
		item = item->leftptrs_[0];
	}
}

void DPState::InsideReordering(vector<int> & result){
	if (!backptrs_.empty()){
		BackPtr & back = backptrs_[0];
		if (action_ == DPState::STRAIGTH){
			back.leftstate->GetReordering(result);
			back.istate->GetReordering(result);
		}
		else if (action_ == DPState::INVERTED){
			back.istate->GetReordering(result);
			back.leftstate->GetReordering(result);
		}
		result.insert(result.begin(), i_);
	}
}

void DPState::GetReordering(vector<int> & result){
	DPState * item = this;
	while (!item->leftptrs_.empty()){
		item->InsideReordering(result);
		item = item->leftptrs_[0];
	}
}


DPState * DPState::Previous(){
	if (action_ == DPState::INIT)
		return NULL;
	else if (action_ == DPState::SHIFT)
		return leftptrs_[0];
	return backptrs_[0].istate;
}

DPState * DPState::GetLeftState(){
	if (action_ == DPState::INIT)
		return NULL;
	return leftptrs_[0];
}

} /* namespace lader */
