/*
 * ddpstate.cc
 *
 *  Created on: Mar 4, 2014
 *      Author: leona
 */




#include "shift-reduce-dp/ddpstate.h"
#include "shift-reduce-dp/flat-tree.h"
#include <cstdlib>
#include <boost/foreach.hpp>
#include <lader/reorderer-model.h>
#include <lader/util.h>
using namespace std;

namespace lader {

// initial state
DDPState::DDPState() : DPState(){
	src_l2_ = src_r2_ = src_rend_ = 0;
	trace_ = NULL;
	nswap_ = 0;
}

// new state
// set src_c_, trg_l_ and trg_r_ latter
DDPState::DDPState(int step, int i, int j, Action action, int i2, int j2) :
		DPState(step, i, j, action) {
	src_l2_ = i2; src_r2_ = src_rend_ = j2;
	trace_ = NULL;
}

DDPState::~DDPState() {
}

void DDPState::MergeWith(DPState * other){
	DPState::MergeWith(other);
	DDPState * lstate = dynamic_cast<DDPState*>(other);
	// prevent parsing failure
	if (lstate && lstate->nswap_ > nswap_)
		nswap_ = lstate->nswap_;
}

// TODO: macro function?
#define CCC (lstate->IsContinuous() && IsContinuous() && lstate->src_r_ == src_l_)
#define CCD (lstate->IsContinuous() && IsContinuous() && lstate->src_r_ < src_l_)
#define CDD (lstate->IsContinuous() && !IsContinuous() && lstate->src_r_ == src_l_)
#define DCD (!lstate->IsContinuous() && IsContinuous() && lstate->src_r2_ == src_l_)
#define DDC (!lstate->IsContinuous() && !IsContinuous() && lstate->src_r_ == src_l_ && src_r_ == lstate->src_l2_ && lstate->src_r2_ == src_l2_)

#define LCCD (llstate->IsContinuous() && IsContinuous() && llstate->src_r_ < src_l_)
#define LCDD (llstate->IsContinuous() && !IsContinuous() && llstate->src_r_ == src_l_)
#define LDCD (!llstate->IsContinuous() && IsContinuous() && llstate->src_r2_ == src_l_)

void DDPState::Take(Action action, DPStateVector & result, bool actiongold,
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
		DDPState * next = dynamic_cast<DDPState*>(Shift());
		if (next->trace_ == NULL){
			next->inside_ = 0;
			next->score_ = score_ + actioncost;
		}
		else{
			next->inside_ = next->trace_->inside_;
			next->score_ = score_ + next->trace_->inside_ + actioncost;
		}
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
	else if (action == SWAP){
		BOOST_FOREACH(DPState * leftstate, leftptrs_){
			DPState * next = Swap(leftstate);
//			next->shiftcost_ = shiftcost_;
			next->actioncost_ = actioncost;
			next->inside_ = inside_ + actioncost;
			next->score_ = score_ + actioncost;
			next->gold_ = gold_ && actiongold;
			// because the left state is swaped out
			BOOST_FOREACH(DPState * s, leftstate->leftptrs_){
				DDPState * llstate = dynamic_cast<DDPState*>(s);
				if (LCCD || LCDD || LDCD)
					next->leftptrs_.push_back(llstate);
			}
			if (next->leftptrs_.empty()){ // swap is actually not allowed
				delete next;
				continue;
			}
			BackPtr back;
//			back.action = action;
//			back.cost = actioncost;
			// maintain the back ptr of the left state for AllActions
			back.lchild = leftstate;
			back.rchild = this;
			next->backptrs_.push_back(back);
			result.push_back(next);
		}
	}
	else if (action == IDLE){
		DPState * next = Idle();
		next->inside_ = inside_ + actioncost;
		next->score_ = score_ + actioncost;
		next->gold_ = gold_ && actiongold;
		next->leftptrs_ = leftptrs_;
		next->actioncost_ = actioncost;
		BackPtr back;
//		back.action = action;
//		back.cost = actioncost;
		back.lchild = NULL;
		back.rchild = this;
		next->backptrs_.push_back(back);
		result.push_back(next);
	}
	else{ // reduce
		BOOST_FOREACH(DPState * leftstate, leftptrs_){
			DDPState * lstate = dynamic_cast<DDPState*>(leftstate);
			 // the left state may not be possible to reduce
			if (CCC || CCD || CDD || DCD || DDC){
				DPState * next = Reduce(leftstate, action);
				next->inside_ = leftstate->inside_ + inside_ + leftstate->shiftcost_ + actioncost;
				next->score_ = leftstate->score_ + inside_ + leftstate->shiftcost_ + actioncost;
				next->gold_ = leftstate->gold_ && gold_ && actiongold;
				next->leftptrs_ = leftstate->leftptrs_;
				next->actioncost_ = actioncost;
				BackPtr back;
//				back.action = action;
//				back.cost = leftstate->shiftcost_ + actioncost;
				back.lchild = leftstate;
				back.rchild = this;
				next->backptrs_.push_back(back);
				result.push_back(next);
			}
		}
	}
}

// either the buffer front or a swaped state
DPState * DDPState::Shift(){
	DDPState * next;
	if (swapped_.empty()){
		next = new DDPState(step_+1, src_rend_, src_rend_+1, SHIFT);
		next->trg_l_ = src_rend_; next->trg_r_ = src_rend_+1;
		next->src_c_ = -1; next->src_rend_ = src_rend_+1;
	}
	else{
		DDPState * top = dynamic_cast<DDPState*>(swapped_.back());
		if (!top)
			THROW_ERROR("Incompetible state in swap" << *swapped_.back() << endl);
		next = new DDPState(step_+1, top->src_l_, top->src_r_, top->action_, // restore the swaped state
									top->src_l2_, top->src_r2_);	// restore the discontinuity as well
		next->trace_ = top;	// this is useful for later stage
		next->swapped_.insert(next->swapped_.begin(), swapped_.begin(), swapped_.end()-1);
		next->trg_l_ = top->trg_l_; next->trg_r_ = top->trg_r_;
		next->src_c_ = -1; next->src_rend_ = src_rend_;
	}
	next->nswap_ = nswap_;
	return next;
}

// a reduced state, possibly discontiuous
DPState * DDPState::Reduce(DPState * leftstate, Action action){
	DDPState * lstate = dynamic_cast<DDPState*>(leftstate);
	if (!lstate)
		THROW_ERROR("left state is NULL" << endl);
	DDPState * next;
	// continuous + continuous = continuous
	if (CCC){
		next = new DDPState(step_+1, lstate->src_l_, src_r_, action);
		next->src_c_ = src_l_;
	}
	// continuous + continuous = discontinuous
	else if (CCD){
		next = new DDPState(step_+1, lstate->src_l_, lstate->src_r_, action, src_l_, src_r_);
		next->src_c_ = -1;
	}
	// continuous + discontinuos = discontinuous
	else if (CDD){
		next = new DDPState(step_+1, lstate->src_l_, src_r_, action, src_l2_, src_r2_);
		next->src_c_ = src_l_;
	}
	// discontinuous + continuous = discontinuous
	else if (DCD){
		next = new DDPState(step_+1, lstate->src_l_, lstate->src_r_, action, lstate->src_l2_, src_r_);
		next->src_c_ = src_l_;
	}
	// discontinuous + discontinuous = continuous
	else if (DDC){
		next = new DDPState(step_+1, lstate->src_l_, src_r2_, action);
		next->src_c_ = src_l_;
	}
	else{
		cerr << "Left: " << endl;
		lstate->PrintTrace(cerr);
		cerr << "This: " << endl;
		this->PrintTrace(cerr);
		THROW_ERROR("Reduce fails: " << (char) action << endl);
	}
	next->src_rend_ = src_rend_;
	next->swapped_.insert(next->swapped_.begin(), swapped_.begin(), swapped_.end());
	next->nswap_ = nswap_;
	if (action == STRAIGTH){
		next->trg_l_ = lstate->trg_l_;		next->trg_r_ = trg_r_;
	}
	else if (action == INVERTED){
		next->trg_l_ = trg_l_;		next->trg_r_ = lstate->trg_r_;
	}
	return next;
}

// a swaped state, push the leftstate on the top of the stack for the swaped states
DPState * DDPState::Swap(DPState * leftstate){
	DDPState * next = new DDPState(step_+1, src_l_, src_r_, SWAP, src_l2_, src_r2_);
	next->swapped_.insert(next->swapped_.begin(), swapped_.begin(), swapped_.end());
	next->swapped_.push_back(leftstate);
	next->src_c_ = src_c_; next->src_rend_ = src_rend_;
	next->trg_l_ = trg_l_;	next->trg_r_ = trg_r_;
	next->nswap_ = nswap_ + 1;
	return next;
}

// an idle state, only change step count
DPState * DDPState::Idle(){
	DDPState * next = new DDPState(step_+1, src_l_, src_r_, IDLE, src_l2_, src_r2_);
	next->swapped_.insert(next->swapped_.begin(), swapped_.begin(), swapped_.end());
	next->src_c_ = src_c_; next->src_rend_ = src_rend_;
	next->trg_l_ = trg_l_;	next->trg_r_ = trg_r_;
	next->nswap_ = nswap_;
	return next;
}

bool DDPState::Allow(const Action & action, const int n){
	DDPState * lstate = dynamic_cast<DDPState*>(GetLeftState());
	if (action == SHIFT)
		return action_ != SWAP && (src_rend_ < n || !swapped_.empty());
	else if (action == SWAP){// this only check the first left state, thus Take may not work
		if (!lstate)
			return false;
		DDPState * llstate = dynamic_cast<DDPState*>(lstate->GetLeftState());
		return llstate && llstate->action_ != INIT	&& (LCCD || LCDD || LDCD)
				&& lstate->src_l_ < src_l_  && src_r_ < n;
	}
	else if (action == IDLE)
		return src_l_ == 0 && src_r_ == n && IsContinuous();
	// this only check the first left state, thus Take may not work
	return lstate && lstate->action_ != INIT && (CCC || CCD || CDD || DCD || DDC);
}


void DDPState::InsideActions(vector<Action> & result) const{
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
	case SWAP:
	case IDLE:
		if (DPState::LeftChild())
			DPState::LeftChild()->InsideActions(result);
		if (DPState::RightChild())
			DPState::RightChild()->InsideActions(result);
		if (DPState::LeftChild() || DPState::RightChild())
			result.push_back(action_);
		else // this is a swaped state, where the action is shift
			result.push_back(SHIFT);
		break;
	default:
		THROW_ERROR("Do not support action " << (char) action_ << endl);
		break;
	}
}

// if this is a swaped state, use trace
DPState * DDPState::LeftChild() const{
	if (trace_ != NULL)
		return trace_->LeftChild();
	return DPState::LeftChild();
}

DPState * DDPState::RightChild() const{
	if (trace_ != NULL)
		return trace_->RightChild();
	return DPState::RightChild();
}

DPState * DDPState::Previous() const{
	if (action_ == INIT)
		return NULL;
	else if (action_ == SHIFT || trace_ != NULL)
		return leftptrs_[0];
	return RightChild();
}

void DDPState::GetReordering(vector<int> & result) const{
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
	case SWAP:
	case IDLE:
		RightChild()->GetReordering(result);
		break;
	case SHIFT:
		for (int i = src_l_ ; i < src_r_ ; i++)
			result.push_back(i);
		break;
	}
}


void DDPState::PrintParse(const vector<string> & strs, ostream & out) const{
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
	case SWAP:
		out << "("<<(char) action_<<" ";
		RightChild()->PrintParse(strs, out);
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

void DDPState::Print(ostream & out) const{
	out << *this;
	BOOST_FOREACH(Span span, GetSignature())
		cerr << " [" << span.first << ", " << span.second << "]";
}

DPStateNode * DDPState::ToFlatTree(){
	DPStateNode * root = new DDPStateNode(0, src_r_, NULL, DPState::INIT);
	root->AddChild(root->Flatten(this));
	return root;
}

} /* namespace lader */
