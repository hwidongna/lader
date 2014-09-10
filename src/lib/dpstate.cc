/*
 * dpstate.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/ddpstate.h"
#include "shift-reduce-dp/flat-tree.h"
#include <cstdlib>
#include <boost/foreach.hpp>
#include <lader/reorderer-model.h>
#include <lader/util.h>
#include <sstream>
using namespace std;

namespace lader {

bool DPState::keep_alternatives_ = false;
ActionVector DPState::ActionFromString(const string & line){
	istringstream iss(line);
	char action;
	vector<Action> result;
	while (iss >> action)
		result.push_back((Action)action);
	return result;
}
// initial state
DPState::DPState() {
	score_ = 0; inside_ = 0; shiftcost_ = 0; actioncost_ = 0;
	step_ = 0;
	src_l_ = 0; src_r_ = 0;
	src_c_ = -1;
	trg_l_ = 0; trg_r_ = 0;
	rank_ = -1;
	action_ = INIT;
	gold_ = true;
	loss_ = 0;
}

// new state
// set src_c_, trg_l_ and trg_r_ latter
DPState::DPState(int step, int i, int j, Action action) {
	score_ = 0; inside_ = 0; shiftcost_ = 0; actioncost_ = 0;
	step_ = step;
	src_l_ = i; src_r_ = j;
	rank_ = -1;
	action_ = action;
	gold_ = false;
	loss_ = 0;
}

DPState::~DPState() {
}

// TODO Is this correct?
bool DPState::operator == (const DPState & other) const {
	if (signature_.size() != other.signature_.size())
		return false;
	return std::equal(signature_.begin(), signature_.end(), other.signature_.begin())
			&& src_l_ == other.src_l_; // do not need to compare j_
}

void DPState::MergeWith(DPState * other){
	if (action_ == SHIFT)
		leftptrs_.push_back(other->leftptrs_[0]);
	else if (action_ != IDLE) // reduce
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
	if (action == SHIFT){
		DPState * next = Shift();
		next->inside_ = 0;
		next->score_ = score_ + actioncost;
		this->shiftcost_ = actioncost;
		next->actioncost_ = actioncost;
		next->gold_ = gold_ && actiongold;
		next->leftptrs_.push_back(this);
		BackPtr back;
//		back.action = action;
//		back.cost = 0;
		back.lchild = NULL;
		back.rchild = NULL;
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
			next->actioncost_ = actioncost;
			BackPtr back;
//			back.action = action;
//			back.cost = leftstate->shiftcost_ + actioncost;
			back.lchild = leftstate;
			back.rchild = this;
			next->backptrs_.push_back(back);
			result.push_back(next);
		}
	}
}


DPState * DPState::Shift(){
	DPState * next = new DPState(step_+1, src_r_, src_r_+1, SHIFT);
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
	if (action == SHIFT)
		return src_r_ < n;
	else if (action == IDLE)
		return src_l_ == 0 && src_r_ == n;
	DPState * leftstate = GetLeftState();
	return leftstate && leftstate->action_ != INIT && leftstate->src_r_ == src_l_;
}

void DPState::InsideActions(ActionVector & result) const{
	switch(action_){
	case INIT:
		break;
	case SHIFT:
		result.push_back(SHIFT);
		for (int i = src_l_+1 ; i < src_r_ ; i++){ // for shift-m > 1
			result.push_back(SHIFT);
			result.push_back(STRAIGTH);
		}
		break;
	case STRAIGTH:
	case INVERTED:
		LeftChild()->InsideActions(result);
		RightChild()->InsideActions(result);
		result.push_back(action_);
		break;
	default:
		THROW_ERROR("Do not support action " << (char) action_ << endl);
		break;
	}
}

void DPState::AllActions(ActionVector & result) const{
	ActionVector tmp;
	InsideActions(tmp);
	result.insert(result.begin(), tmp.begin(), tmp.end());
	if (GetLeftState())
		GetLeftState()->AllActions(result);
}

void DPState::GetReordering(vector<int> & result) const{
	switch(action_){
	case INIT:
		break;
	case STRAIGTH:
		LeftChild()->GetReordering(result);
		RightChild()->GetReordering(result);
		break;
	case INVERTED:
		RightChild()->GetReordering(result);
		LeftChild()->GetReordering(result);
		break;
	case SHIFT:
		for (int i = src_l_ ; i < src_r_ ; i++)
			result.push_back(i);
		break;
	}
}

void DPState::SetSignature(int max){
	if (!signature_.empty())
		THROW_ERROR("Signature exists!" << *this)
	DPState * state = this;
	for (int i = 0 ; i < max && state->action_ != INIT ; i++){
		signature_.push_back(state->GetTrgSpan());
		state = state->GetLeftState();
	}
}
DPState * DPState::Previous() const{
	if (action_ == INIT)
		return NULL;
	else if (action_ == SHIFT)
		return leftptrs_[0];
	return RightChild();
}

DPState * DPState::GetLeftState() const{
	if (action_ == INIT)
		return NULL;
	return leftptrs_[0];
}

DPState * DPState::LeftChild() const{
	if (action_ == INIT || action_ == SHIFT)
		return NULL;
	return backptrs_[0].lchild;
}

DPState * DPState::RightChild() const{
	if (action_ == INIT || action_ == SHIFT)
		return NULL;
	return backptrs_[0].rchild;
}

void DPState::PrintParse(const vector<string> & strs, ostream & out) const{
	switch(action_){
	case INIT:
		break;
	case STRAIGTH:
	case INVERTED:
		out << "("<<(char) action_<<" ";
		LeftChild()->PrintParse(strs, out);
		out << " ";
		RightChild()->PrintParse(strs, out);
		out << ")";
		break;
	case SHIFT:
		out << "(" <<(char)  action_ << " " << GetTokenWord(strs[GetSrcL()]) <<")";
		break;
	}
}

void DPState::PrintTrace(ostream & out) const{
	this->Print(out);
	out << endl;
	if (Previous())
		Previous()->PrintTrace(out);
}

void DPState::Print(ostream & out) const{
	out << *this;
	BOOST_FOREACH(Span span, GetSignature())
		cerr << " [" << span.first << ", " << span.second << "]";
}
DPStateNode * DPState::ToFlatTree(){
	DPStateNode * root = new DPStateNode(0, src_r_, NULL, DPState::INIT);
	root->AddChild(root->Flatten(this));
	return root;
}

unsigned long DPState::GetNumDerivations(){
	unsigned long count = leftptrs_.size();
	if (LeftChild())
		count += LeftChild()->GetNumDerivations();
	if (RightChild())
		count += RightChild()->GetNumDerivations();
	return count;
}

} /* namespace lader */
