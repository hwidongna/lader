/*
 * ddpstate.cc
 *
 *  Created on: Mar 4, 2014
 *      Author: leona
 */




#include "shift-reduce-dp/ddpstate.h"
#include <cstdlib>
#include <boost/foreach.hpp>
#include <lader/reorderer-model.h>
#include <lader/util.h>
using namespace std;

namespace lader {

// initial state
DDPState::DDPState() : DPState(){
	src_l2_ = src_r2_ = src_rend_ = 0;
}

// new state
// set src_c_, trg_l_ and trg_r_ latter
DDPState::DDPState(int step, int i, int j, Action action, int i2, int j2) :
		DPState(step, i, j, action) {
	src_l2_ = i2; src_r2_ = src_rend_ = j2;
}

DDPState::~DDPState() {
}

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
						*next, SHIFT, model->GetFeatureIds(), model->GetAdd());
				shiftcost = model->ScoreFeatureVector(*fvi);
				delete fvi;
			}
			// shifted->inside_ = 0;
			shifted->score_ = next->score_ + shiftcost;
			DPState * reduced = shifted->Reduce(next, STRAIGTH);
			double reducecost = 0;
			if (model != NULL && feature_gen != NULL && sent != NULL){
				shifted->leftptrs_.push_back(next);
				// TODO: reflect the maxterm information for feature generation?
				const FeatureVectorInt * fvi = feature_gen->MakeStateFeatures(*sent,
						*shifted, STRAIGTH, model->GetFeatureIds(), model->GetAdd());
				reducecost = model->ScoreFeatureVector(*fvi);
				delete fvi;
			}
			// because shifted->inside_ == 0, do not need to add them
			reduced->inside_ = next->inside_ + shiftcost + reducecost;
			reduced->score_ = next->score_ + shiftcost + reducecost;
			delete shifted; delete next; // intermidiate states, c++ syntax sucks!
			next = reduced;
			next->action_ = DDPState::SHIFT; // it is actuall a shifted state
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
	else if (action == SWAP){
		BOOST_FOREACH(DPState * leftstate, leftptrs_){
			DPState * next = Swap(leftstate);
			next->shiftcost_ = leftstate->shiftcost_;
			next->inside_ = leftstate->inside_ + actioncost;
			next->score_ = leftstate->score_ + actioncost;
			next->gold_ = leftstate->gold_ && gold_ && actiongold;
			next->leftptrs_ = leftstate->leftptrs_;
			BackPtr back;
			back.action = action;
			back.cost = actioncost;
			back.leftstate = leftstate;
			back.istate = this;
			next->backptrs_.push_back(back);
			result.push_back(next);
		}
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

// either the buffer front or a swaped state
DPState * DDPState::Shift(){
	DDPState * next;
	if (swaped_.empty()){
		next = new DDPState(step_+1, src_rend_, src_rend_+1, SHIFT);
		next->trg_l_ = src_rend_; next->trg_r_ = src_rend_+1;
		next->src_c_ = -1; next->src_rend_ = src_rend_+1;
	}
	else{
		DPState * top = swaped_.back();
		next = new DDPState(step_+1, top->src_l_, top->src_r_, SHIFT);
		next->swaped_.insert(next->swaped_.begin(), swaped_.begin(), swaped_.end()-1);
		next->trg_l_ = top->trg_l_; next->trg_r_ = top->trg_r_;
		next->src_c_ = -1; next->src_rend_ = src_rend_;
	}
	return next;
}

// a reduced state, possibly discontiuous
DPState * DDPState::Reduce(DPState * leftstate, Action action){
	DDPState * next;
	// continuous + continuous = continuous
	if (leftstate->IsContinuous() && IsContinuous() && leftstate->src_r_ == src_l_){
		next = new DDPState(step_+1, leftstate->src_l_, src_r_, action);
		next->src_c_ = src_l_;
	}
	// continuous + continuous = discontinuous
	else if (leftstate->IsContinuous() && IsContinuous()){
		next = new DDPState(step_+1, leftstate->src_l_, leftstate->src_r_, action, src_l_, src_r_);
		next->src_c_ = -1;
	}
	// continuous + discontinuos = discontinuous
	else if (leftstate->IsContinuous()){
		next = new DDPState(step_+1, leftstate->src_l_, src_r_, action, src_l2_, src_r2_);
		next->src_c_ = src_l_;
	}
	// discontinuous + continuous = discontinuous
	else if (IsContinuous()){
		DDPState * lstate = dynamic_cast<DDPState*>(leftstate);
		next = new DDPState(step_+1, lstate->src_l_, lstate->src_r_, action, lstate->src_l2_, src_r2_);
		next->src_c_ = src_l_;
	}
	// discontinuous + discontinuous = continuous
	else{
		DDPState * lstate = dynamic_cast<DDPState*>(leftstate);
		next = new DDPState(step_+1, lstate->src_l_, src_r_, action);
		next->src_c_ = src_l_;
	}
	next->src_rend_ = src_rend_;
	next->swaped_.insert(next->swaped_.begin(), swaped_.begin(), swaped_.end());
	if (action == STRAIGTH){
		next->trg_l_ = leftstate->trg_l_;		next->trg_r_ = trg_r_;
	}
	else if (action == INVERTED){
		next->trg_l_ = trg_l_;		next->trg_r_ = leftstate->trg_r_;
	}
	return next;
}

// a swaped state, push the leftstate in a temporary
DPState * DDPState::Swap(DPState * leftstate){
	DDPState * next = new DDPState(step_+1, src_l_, src_r_, SWAP);
	next->swaped_.insert(next->swaped_.begin(), swaped_.begin(), swaped_.end());
	next->swaped_.push_back(leftstate);
	next->src_c_ = src_c_; next->src_rend_ = src_rend_;
	next->trg_l_ = trg_l_;	next->trg_r_ = trg_r_;
	return next;

}
bool DDPState::Allow(const Action & action, const int n){
	DDPState * lstate = dynamic_cast<DDPState*>(GetLeftState());
	if (action == SHIFT)
		return src_rend_ < n || !swaped_.empty();
	else if (action == SWAP)
		return lstate && lstate->action_ != INIT
				&& lstate->IsContinuous() && IsContinuous() && lstate->src_l_ < src_l_;
	return lstate && lstate->action_ != INIT
			&& (lstate->src_r_ == src_l_
				|| (lstate->IsContinuous() && IsContinuous()));
}

} /* namespace lader */
