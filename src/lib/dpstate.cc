/*
 * dpstate.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/dpstate.h"
#include <cstdlib>
#include <boost/foreach.hpp>
#include <lader/reorderer-model.h>
using namespace std;

namespace lader {

// initial state
DPState::DPState() {
	score_ = 0; inside_ = 0; shiftcost_ = 0;
	step_ = 0;
	src_l_ = 0; src_r_ = 0;
	src_c_ = -1;
	trg_l_ = 0; trg_r_ = 0;
	rank_ = -1;
	action_ = DPState::INIT;
	gold_ = true;
}

// new state
// set src_c_, trg_l_ and trg_r_ latter
DPState::DPState(int step, int i, int j, Action action) {
	score_ = 0, inside_ = 0, shiftcost_ = 0;
	step_ = step;
	src_l_ = i, src_r_ = j;
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
void DPState::Take(Action action, DPStateVector & result, bool actiongold,
		int maxterm, ReordererModel * model, const FeatureSet * feature_gen,
		const Sentence * sent) {
	double 	actioncost = 0;
	if (model != NULL && feature_gen != NULL && sent != NULL){
		const FeatureVectorInt * fvi = feature_gen->MakeStateFeatures(*sent,
				*this, action, model->GetFeatureIds(), model->GetAdd());
		actioncost = model->ScoreFeatureVector(*fvi);
		delete fvi;
	}
	if (action == DPState::SHIFT){
		DPState * next = Shift();
		next->inside_ = 0;
		next->score_ = score_ + actioncost;
		while (maxterm-- > 1){ // shift more than one element monotonically
			DPState * shifted = next->Shift();
			double shiftcost = 0;
			if (model != NULL && feature_gen != NULL && sent != NULL){
				next->leftptrs_.push_back(this);
				// TODO: reflect the maxterm information for feature generation
				const FeatureVectorInt * fvi = feature_gen->MakeStateFeatures(*sent,
						*next, DPState::SHIFT, model->GetFeatureIds(), model->GetAdd());
				shiftcost = model->ScoreFeatureVector(*fvi);
				delete fvi;
			}
			// shifted->inside_ = 0;
			shifted->score_ = next->score_ + shiftcost;
			DPState * reduced = shifted->Reduce(next, DPState::STRAIGTH);
			double reducecost = 0;
			if (model != NULL && feature_gen != NULL && sent != NULL){
				shifted->leftptrs_.push_back(next);
				// TODO: reflect the maxterm information for feature generation?
				const FeatureVectorInt * fvi = feature_gen->MakeStateFeatures(*sent,
						*shifted, DPState::STRAIGTH, model->GetFeatureIds(), model->GetAdd());
				reducecost = model->ScoreFeatureVector(*fvi);
				delete fvi;
			}
			// because shifted->inside_ == 0, do not need to add them
			reduced->inside_ = next->inside_ + shiftcost + reducecost;
			reduced->score_ = next->score_ + shiftcost + reducecost;
			delete shifted; delete next; // intermidiate states, c++ syntax sucks!
			next = reduced;
			next->action_ = DPState::SHIFT; // it is actuall a shifted state
		}
		shiftcost_ = actioncost;
		next->gold_ = gold_ && actiongold;
		next->leftptrs_.push_back(this);
		BackPtr back;
		back.action = action;
		back.cost = 0;
		back.leftstate = NULL;
		back.istate = NULL;
		next->backptrs_.push_back(back);
		result.push_back(next);
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
}


DPState * DPState::Shift(){
	DPState * next = new DPState(step_+1, src_r_, src_r_+1, DPState::SHIFT);
	next->trg_l_ = src_r_; next->trg_r_ = src_r_+1;
	next->src_c_ = -1;
	return next;
}
DPState * DPState::Reduce(DPState * leftstate, Action action){
	DPState * next = new DPState(step_+1, leftstate->src_l_, src_r_, action);
	next->src_c_ = src_l_;
	if (action == STRAIGTH){
		next->trg_l_ = leftstate->trg_l_;		next->trg_r_ = trg_r_;
	}
	else if (action == INVERTED){
		next->trg_l_ = trg_l_;		next->trg_r_ = leftstate->trg_r_;
	}
	return next;
}

bool DPState::Allow(const Action & action, const int n){
	if (action == DPState::SHIFT)
		return src_r_ < n;
	DPState * leftstate = GetLeftState();
	return leftstate && leftstate->action_ != INIT
			&& (leftstate->src_r_ == src_l_ || src_r_ == leftstate->src_l_);
}

void DPState::InsideActions(vector<Action> & result){
	switch(action_){
	case DPState::INIT:
		break;
	case DPState::SHIFT:
		result.push_back(DPState::SHIFT);
		for (int i = src_l_+1 ; i < src_r_ ; i++){ // for shift-m > 1
			result.push_back(DPState::SHIFT);
			result.push_back(DPState::STRAIGTH);
		}
		break;
	case DPState::STRAIGTH:
	case DPState::INVERTED:
		LeftChild()->InsideActions(result);
		RightChild()->InsideActions(result);
		result.push_back(action_);
		break;
	}
}

void DPState::AllActions(vector<Action> & result){
	DPState * state = this;
	while (state){
		vector<Action> tmp;
		state->InsideActions(tmp);
		result.insert(result.begin(), tmp.begin(), tmp.end());
		state = state->GetLeftState();
	}
}

void DPState::GetReordering(vector<int> & result){
	switch(action_){
	case DPState::INIT:
		break;
	case DPState::STRAIGTH:
		LeftChild()->GetReordering(result);
		RightChild()->GetReordering(result);
		break;
	case DPState::INVERTED:
		RightChild()->GetReordering(result);
		LeftChild()->GetReordering(result);
		break;
	case DPState::SHIFT:
		for (int i = src_l_ ; i < src_r_ ; i++)
			result.push_back(i);
		break;
	}
}

void DPState::SetSignature(int max){
	if (!signature_.empty())
		THROW_ERROR("Signature exists!" << *this)
	DPState * leftstate = this;
	for (int i = 0 ; i < max && leftstate->action_ != DPState::INIT ; i++){
		signature_.push_back(leftstate->GetTrgSpan());
		leftstate = leftstate->GetLeftState();
	}
}
DPState * DPState::Previous(){
	if (action_ == DPState::INIT)
		return NULL;
	else if (action_ == DPState::SHIFT)
		return leftptrs_[0];
	return RightChild();
}

DPState * DPState::GetLeftState() const{
	if (action_ == DPState::INIT)
		return NULL;
	return leftptrs_[0];
}

DPState * DPState::LeftChild() const{
	if (action_ == DPState::INIT || action_ == DPState::SHIFT)
		return NULL;
	return backptrs_[0].leftstate;
}

DPState * DPState::RightChild() const{
	if (action_ == DPState::INIT || action_ == DPState::SHIFT)
		return NULL;
	return backptrs_[0].istate;
}

inline string GetTokenWord(const string & str) {
    ostringstream oss;
    for(int i = 0; i < (int)str.length(); i++) {
        switch (str[i]) {
            case '(': oss << "-LRB-"; break;
            case ')': oss << "-RRB-"; break;
            case '[': oss << "-LSB-"; break;
            case ']': oss << "-RSB-"; break;
            case '{': oss << "-LCB-"; break;
            case '}': oss << "-RCB-"; break;
            default: oss << str[i]; break;
        }
    }
    return oss.str();
}

void DPState::PrintParse(const vector<string> & strs, ostream & out) const{
	switch(action_){
	case DPState::INIT:
		break;
	case DPState::STRAIGTH:
	case DPState::INVERTED:
		out << "("<<action_<<" ";
		LeftChild()->PrintParse(strs, out);
		out << " ";
		RightChild()->PrintParse(strs, out);
		out << ")";
		break;
	case DPState::SHIFT:
		out << "(" << action_ << " " << GetTokenWord(strs[GetSrcL()]) <<")";
		break;
	}
}
} /* namespace lader */
