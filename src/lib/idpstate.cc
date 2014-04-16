/*
 * idpstate.cc
 *
 *  Created on: Mar 31, 2014
 *      Author: leona
 */




#include "shift-reduce-dp/idpstate.h"
#include "shift-reduce-dp/flat-tree.h"
#include <cstdlib>
#include <boost/foreach.hpp>
#include <lader/reorderer-model.h>
#include <lader/util.h>
#include <lader/ranks.h>
using namespace std;

namespace lader {

// initial state
IDPState::IDPState() : DPState(){
	nins_ = ndel_ = 0;
}

// new state
// set src_c_, trg_l_ and trg_r_ latter
IDPState::IDPState(int step, int i, int j, Action action) :
		DPState(step, i, j, action) {
	nins_ = ndel_ = 0;
}

IDPState::~IDPState() {
}

void IDPState::MergeWith(DPState * other){
	DPState::MergeWith(other);
	IDPState * lstate = dynamic_cast<IDPState*>(other);
	// prevent parsing failure
	if (lstate && lstate->nins_ > nins_)
		nins_ = lstate->nins_;
	if (lstate && lstate->ndel_ > ndel_)
		ndel_ = lstate->ndel_;
}

void IDPState::Take(Action action, DPStateVector & result, bool actiongold,
		int maxterm, ReordererModel * model, const FeatureSet * feature_gen,
		const Sentence * sent) {
	double 	actioncost = 0;
	if (model != NULL && feature_gen != NULL && sent != NULL){
		const FeatureVectorInt * fvi = feature_gen->MakeStateFeatures(*sent,
				*this, action, model->GetFeatureIds(), model->GetAdd());
		actioncost = model->ScoreFeatureVector(*fvi);
		delete fvi;
	}
	DPState * next;
	BackPtr back;
	switch(action){
	// terminal
	case SHIFT:
		next = Shift();
		next->inside_ = 0;
		next->score_ = score_ + actioncost;
		shiftcost_ = actioncost; // to be used later in reduce
		next->gold_ = gold_ && actiongold;
		next->leftptrs_.push_back(this);
		back.action = action;
		back.cost = 0;
		back.lchild = NULL;
		back.rchild = NULL;
		next->backptrs_.push_back(back);
		result.push_back(next);
		break;
	// unary operations
	case INSERT_L:
	case INSERT_R:
	case IDLE:
		next = Insert(action);
		next->inside_ = inside_ + actioncost;
		next->score_ = score_ + actioncost;
		next->gold_ = gold_ && actiongold;
		next->leftptrs_ = leftptrs_; // leftstates remains same
		back.action = action;
		back.cost = actioncost;
		back.lchild = NULL;
		back.rchild = this; // only has right child
		next->backptrs_.push_back(back);
		result.push_back(next);
		break;
	// binary operations (reduce)
	case DELETE_L:
	case DELETE_R:
	case STRAIGTH:
	case INVERTED:
		BOOST_FOREACH(DPState * leftstate, leftptrs_){
			next = Reduce(leftstate, action);
			if (!next)
				continue;
			next->inside_ = leftstate->inside_ + inside_ + leftstate->shiftcost_ + actioncost;
			next->score_ = leftstate->score_ + inside_ + leftstate->shiftcost_ + actioncost;
			next->gold_ = leftstate->gold_ && gold_ && actiongold;
			next->leftptrs_ = leftstate->leftptrs_;
			back.action = action;
			back.cost = leftstate->shiftcost_ + actioncost;
			back.lchild = leftstate;
			back.rchild = this;
			next->backptrs_.push_back(back);
			result.push_back(next);
		}
		break;
	default:
		THROW_ERROR("Unsupported action " << (char)action << " is taken" << endl);
		break;
	}
}

DPState * IDPState::Shift(){
	IDPState * next = new IDPState(step_+1, src_r_, src_r_+1, SHIFT);
	next->trg_l_ = src_r_; next->trg_r_ = src_r_+1;
	next->src_c_ = -1;
	next->nins_ = nins_; next->ndel_ = ndel_;
	return next;
}


// a reduced/deleted state
DPState * IDPState::Reduce(DPState * leftstate, Action action){
	if ((action == DELETE_L && leftstate->action_ != SHIFT)
		|| (action == DELETE_R && action_ != SHIFT))
		return NULL;
	IDPState * next = new IDPState(step_+1, leftstate->src_l_, src_r_, action);
	next->src_c_ = src_l_;
	if (action == STRAIGTH){
		next->trg_l_ = leftstate->trg_l_;		next->trg_r_ = trg_r_;
	}
	else if (action == INVERTED){
		next->trg_l_ = trg_l_;		next->trg_r_ = leftstate->trg_r_;
	}
	else{
		next->trg_l_ = leftstate->trg_l_;	next->trg_r_ = trg_r_;
	}
	next->nins_ = nins_;
	if (action == DELETE_L || action == DELETE_R)
		next->ndel_ = ndel_ + 1;
	else
		next->ndel_ = ndel_;
	return next;
}

// an insert/idle state
DPState * IDPState::Insert(Action action){
	IDPState * next = new IDPState(step_+1, src_l_, src_r_, action);
	next->src_c_ = src_c_;
	next->trg_l_ = trg_l_;	next->trg_r_ = trg_r_;
	if (action == IDLE)
		next->nins_ = nins_;
	else
		next->nins_ = nins_ + 1;
	next->ndel_ = ndel_;
	return next;
}

bool IDPState::Allow(const Action & action, const int n){
	DPState * leftstate = GetLeftState();
	if (action == SHIFT)
		return src_r_ < n;
	else if (action == IDLE)
		return src_l_ == 0 && src_r_ == n;
	else if (action == INSERT_L || action == INSERT_R) // do not allow consecutive insert
		return action_ != INIT && action_ != INSERT_L && action_ != INSERT_R;
	else if (action == DELETE_L)
		// this only check whether the first left state is shifted
		// thus, check the rest left state again when actually taking action
		return leftstate && leftstate->action_ == SHIFT;
	else if (action == DELETE_R)
		return leftstate && leftstate ->action_ != INIT && action_ == SHIFT;
	return leftstate && leftstate->action_ != INIT && leftstate->src_r_ == src_l_;
}


void IDPState::InsideActions(ActionVector & result) const{
	switch(action_){
	case INIT:
		break;
	case SHIFT:
		result.push_back(SHIFT);
		break;
	case INSERT_L:
	case INSERT_R:
	case DELETE_L:
	case DELETE_R:
	case STRAIGTH:
	case INVERTED:
	case IDLE:
		if (DPState::LeftChild())
			DPState::LeftChild()->InsideActions(result);
		if (DPState::RightChild())
			DPState::RightChild()->InsideActions(result);
		result.push_back(action_);
		break;
	default:
		THROW_ERROR("Do not support action " << (char) action_ << endl);
		break;
	}
}

void IDPState::GetReordering(vector<int> & result) const{
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
	case INSERT_L:
	case INSERT_R:
	case DELETE_L:
	case IDLE:
		if (action_ == INSERT_L)
			result.push_back(-1);
		RightChild()->GetReordering(result);
		if (action_ == INSERT_R)
			result.push_back(-1);
		break;
	case DELETE_R:
		LeftChild()->GetReordering(result);
		break;
	case SHIFT:
		for (int i = src_l_ ; i < src_r_ ; i++)
			result.push_back(i);
		break;
	}
}

void IDPState::PrintParse(const vector<string> & strs, ostream & out) const{
	switch(action_){
	case INIT:
		break;
	case STRAIGTH:
	case INVERTED:
	case DELETE_L:
	case DELETE_R:
		out << "("<<(char) action_<<" ";
		LeftChild()->PrintParse(strs, out);
		out << " ";
		RightChild()->PrintParse(strs, out);
		out << ")";
		break;
	case INSERT_L:
	case INSERT_R:
		out << "("<<(char) action_<<" ";
		if (action_ == INSERT_L)
			out << "-1 ";
		RightChild()->PrintParse(strs, out);
		if (action_ == INSERT_R)
			out << " -1";
		out << ")";
		break;
	case IDLE:
		RightChild()->PrintParse(strs, out);
		break;
	case SHIFT:
		out << "(" <<(char)  action_ << " " << GetTokenWord(strs[GetSrcL()]) <<")";
		break;
	}
}

void IDPState::Print(ostream & out) const{
	out << *this;
}

// compare signature and action if inserted
bool IDPState::operator == (const DPState & other) const {
	const IDPState * dother = dynamic_cast<const IDPState *>(&other);
	if (!dother){
		this->Print(cerr); cerr << endl;
		other.Print(cerr); cerr << endl;
		THROW_ERROR("Incompatible comparison " << this->name () << " != " << other.name())
	}
	// if either this or other state is not inserted, they are not equal
	if ((action_ != INSERT_L ^ dother->action_ != INSERT_L)
			|| (action_ != INSERT_R ^ dother->action_ != INSERT_R))
		return false;
	return DPState::operator ==(other);
}

} /* namespace lader */
