/*
 * parser.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/parser.h"
#include <boost/foreach.hpp>
#include <iostream>
#include <lader/reorderer-model.h>

using namespace std;

namespace lader {

Parser::Parser(): nstates_(0), nedges_(0), nuniq_(0), beamsize_(0), verbose_(0)  {
	actions_.push_back(DPState::SHIFT);
	actions_.push_back(DPState::STRAIGTH);
	actions_.push_back(DPState::INVERTED);
}

Parser::~Parser() {
	BOOST_FOREACH(DPStateVector & b, beams_)
		BOOST_FOREACH(DPState * state, b)
			delete state;
}

// According to the reference sequence of actions,
// return the guided search result if possible.
// It may not be allowed some action in the reference sequence,
// e.g. because there are too many swap/insert actions,
// in such case, return NULL
DPState * Parser::GuidedSearch(const ActionVector & refseq, int n){
	int max_step = refseq.size() + 1;
	beams_.resize(max_step, DPStateVector());
	DPState * state = InitialState();
	beams_[0].push_back(state); // initial state
	for (int step = 1 ; step < max_step ; step++){
		DPState::Action action = refseq[step-1];
		if (Allow(state, action, n))
			state->Take(action, beams_[step], true);
		else {// if threre are too many swap/insert actions
			return NULL;
		}
		state = beams_[step][0];
	}
	return state;
}
void Parser::Search(ShiftReduceModel & model,
		const FeatureSet & feature_gen, const Sentence & sent,
		Result * result, const ActionVector * refseq,
		const string * update) {
	DPStateVector golds(2*sent[0]->GetNumWords(), NULL);
	DynamicProgramming(golds, model, feature_gen, sent, refseq);
	if (result && refseq){
		DPStateVector simgolds;
		CompleteGolds(simgolds, golds, model, feature_gen, sent, refseq);
		Update(golds, result, refseq, update);
		// clean-up the rest gold items using refseq
		BOOST_FOREACH(DPState * gold, simgolds)
			delete gold;
	}
}

void Parser::DynamicProgramming(DPStateVector & golds, ShiftReduceModel & model,
		const FeatureSet & feature_gen, const Sentence & sent,
		const ActionVector * refseq) {
	int n = sent[0]->GetNumWords();
	int max_step = golds.size();
	beams_.resize(max_step, DPStateVector());
	DPState * state = InitialState();
	beams_[0].push_back(state); // initial state

	golds[0] = beams_[0][0]; // initial state
	for (int step = 1 ; step < max_step ; step++){
		if (verbose_ >= 2)
			cerr << "step " << step << endl;
		DPStateQueue q;
		BOOST_FOREACH(DPState * old, beams_[step-1]){
			if (verbose_ >= 2){
				DDPState * dold = dynamic_cast<DDPState*>(old);
				cerr << "OLD: " << (dold ? *dold : *old);
				BOOST_FOREACH(Span span, old->GetSignature())
					cerr << " [" << span.first << ", " << span.second << "]";
				cerr << endl;
			}
			// iterate over actions
			BOOST_FOREACH(DPState::Action action, actions_){
				if (!Allow(old, action, n))
					continue;
				bool actiongold = (refseq != NULL
						&& (action == DPState::IDLE || action == (*refseq)[step-1]));
				DPStateVector stateseq;
				// take an action, see below shift-m
				old->Take(action, stateseq, actiongold, 1,
						&model, &feature_gen, &sent);
				BOOST_FOREACH(DPState * next, stateseq){
					next->SetSignature(model.GetMaxState());
					q.push(next);
					if (verbose_ >= 2){
						cerr << "  NEW: " << *next;
						BOOST_FOREACH(Span span, next->GetSignature())
							cerr << " [" << span.first << ", " << span.second << "]";
						cerr << endl;
					}
				}
			}
		}
		// shift m+1 element monotonically, i.e., shift-{shift-straight}*m
		// from an oldstep (=step-2*m-1) if possible
		// the new state is identical to the state that apply a sequence of actions
		// from the old state if exist. however, it may not exist due to the pruning.
		// therefore, this is the second chance to revive the state we needed often.
		for (int m = 1 ; step > 2*m && m < model.GetMaxTerm() ; m++){
			// from the proper previous step
			int oldstep = step-2*m-1;
			BOOST_FOREACH(DPState * old, beams_[oldstep]){
				if (old->GetSrcREnd()+m >= n)
					continue;
				if (verbose_ >= 2){
					DDPState * dold = dynamic_cast<DDPState*>(old);
					cerr << "OLD: " << (dold ? *dold : *old);
					BOOST_FOREACH(Span span, old->GetSignature())
						cerr << " [" << span.first << ", " << span.second << "]";
					cerr << endl;
				}
				DPState::Action action = DPState::SHIFT;
				bool actiongold = (refseq != NULL && action == (*refseq)[oldstep]);
				// get the correct judgement of actiongold
				for (int i = 0 ; refseq && i < 2*m-1; i += 2){
					actiongold &= DPState::SHIFT == (*refseq)[oldstep+i+1];
					actiongold &= DPState::STRAIGTH == (*refseq)[oldstep+i+2];
				}
				DPStateVector stateseq;
				old->Take((DPState::Action) action, stateseq, actiongold, m+1,
						&model, &feature_gen, &sent);
				if (stateseq.size() > 1)
					THROW_ERROR("shift-" << m+1 << " produces more than one next state")
				DPState * next = stateseq.back(); // only one item
				next->SetSignature(model.GetMaxState());
				q.push(next);
				if (verbose_ >= 2){
					cerr << "  NEW: " << *next;
					BOOST_FOREACH(Span span, next->GetSignature())
						cerr << " [" << span.first << ", " << span.second << "]";
					cerr << endl;
				}
			}
		}
		nedges_ += q.size(); // number of created states (correlates with running time)
		if (verbose_ >= 2)
			cerr << "produce " << q.size() << " items" << endl;

		DPStateMap tmp;
		int rank = 0;
		for (int k = 0 ; !q.empty() && (!beamsize_ || k < beamsize_) ; k++){
			DPState * top = q.top(); q.pop();
			DPStateMap::iterator it = tmp.find(*top); // utilize hash() and operator==()
			if (it == tmp.end()){
				tmp[*top] = top;
				beams_[step].push_back(top);
				top->SetRank(rank++);
				if (top->IsGold())
					golds[step] = top;
				if (verbose_ >= 2){
					cerr << *top << " :";
					BOOST_FOREACH(Span span, top->GetSignature())
						cerr << " [" << span.first << ", " << span.second << "]";
					cerr << endl;
				}

			}
			else{
				it->second->MergeWith(top);
				if (top->IsGold() && verbose_ >= 2) // fall-off by merge
					cerr << "GOLD at " << k << " MERGED with " << it->second->GetRank() << endl;
				delete top;
			}
		}
		nstates_ += beams_[step].size(); // number of survived states
		if (verbose_ >= 2)
			cerr << "survive " << beams_[step].size() << " items" << endl;
		nuniq_ += rank;
		// clean-up the rest
		while (!q.empty()){
			if (q.top()->IsGold())
				golds[step] = NULL;
			delete q.top(); q.pop();
		}
	}
}

void Parser::CompleteGolds(DPStateVector & simgolds, DPStateVector & golds,
		ShiftReduceModel & model, const FeatureSet & feature_gen,
		const Sentence & sent, const ActionVector * refseq) {
	int n = sent[0]->GetNumWords();
	if (verbose_ >= 2){
		cerr << "Reference:";
		BOOST_FOREACH(DPState::Action action, *refseq)
			cerr << " " << (char) action;
		cerr << endl;
	}
	// complete the rest gold items using refseq
	for (int step = 1 ; step <= refseq->size() && step < golds.size() ; step++){
		if (golds[step] == NULL){
			DPState::Action action = (*refseq)[step-1];
			DPState * state = golds[step-1];
			DPStateVector tmp;
			if (state->Allow(action, n)) // take an action for golds
				state->Take(action, tmp, true, 1,
						&model, &feature_gen, &sent);
			else{
				state->PrintTrace(cerr);
				THROW_ERROR("Bad action " << (char)action << endl);
			}
			// may have more than one left state if merged?
			simgolds.push_back(tmp[0]);
			for (int i = 1 ; i < tmp.size() ; i++)
				delete tmp[i];
			golds[step] = simgolds.back(); // only one next item
			if (verbose_ >= 2)
				cerr << "SIMGOLD: " << *golds[step] << endl;
		}
		else if (verbose_ >= 2)
			cerr << "GOLD: " << *golds[step] << endl;
	}
	// padding IDLE states if possible
	for (int step = refseq->size()+1 ; step < golds.size() ; step++){
		DPState::Action action = DPState::IDLE;
		DPState * state = golds[step-1];
		DPStateVector tmp;
		if (state->Allow(action, n)) // take an action for golds
			state->Take(action, tmp, true, 1,
					&model, &feature_gen, &sent);
		else
			break;
		// may have more than one left state if merged?
		simgolds.push_back(tmp[0]);
		for (int i = 1 ; i < tmp.size() ; i++)
			delete tmp[i];
		golds[step] = simgolds.back(); // only one next item
	}
}
// support naive, early, max update for perceptron
void Parser::Update(DPStateVector & golds, Result * result,
		const ActionVector * refseq, const string * update) {
	int earlypos = -1, latepos, largepos, maxpos = -1, naivepos;
	if (verbose_ >= 2){
		cerr << "update strategy: " << *update << endl;
		cerr << "gold size: " << golds.size() << ", beam size: " << beams_.size() << endl;
	}
	double maxdiff = 0;
	if (golds.size() != beams_.size())
		THROW_ERROR("gold size " << golds.size() << " != beam size" << beams_.size() << endl);
	for (int step = 1 ; step < (int)beams_.size() ; step++){
		const DPState * best = GetBeamBest(step); // update against best
		const DPState * gold = golds[step];
		if (!best)
			THROW_ERROR("Parsing failure at step " << step << endl);
		if (!gold) // no more update is possible
			break;
		naivepos = step;
		if (verbose_ >= 2){
			cerr << "BEST: " << *best << endl;
			cerr << "GOLD: " << *gold << endl;
		}
		if (gold->GetRank() < 0){
			if (*update == "early"){ // fall-off
				best->AllActions(result->actions);
				result->step = step;
				result->score = 0;
				if (verbose_ >= 2)
					cerr << "Early update at step " << result->step << endl;
				return;
			}
			if (earlypos < 0)
				earlypos = step;
		}
		if (*update == "max"){
			double mdiff = best->GetScore() - gold->GetScore();
			if (mdiff >= maxdiff){
				maxdiff = mdiff;
				maxpos = step;
			}
		}
	}
	if (*update == "max" && maxpos > 0){
		beams_[maxpos][0]->AllActions(result->actions);
		result->step = maxpos;
		result->score = 0;
		if (verbose_ >= 2)
			cerr << "Max update at step " << result->step << endl;
		return;
	}
	SetResult(result, beams_[naivepos][0]);
}

void Parser::SetResult(Parser::Result * result, const DPState * goal){
	if (!result || !goal)
		THROW_ERROR("SetResult does not accept NULL arguments " << endl)
	goal->GetReordering(result->order);
	result->step = goal->GetStep();
	goal->AllActions(result->actions);
	result->score = goal->GetScore();
}

void Parser::GetKbestResult(vector<Result> & kbest){
	for (int k = 0 ; GetKthBest(k) != NULL ; k++){
		Result result;
		SetResult(&result, GetKthBest(k));
		kbest.push_back(result);
	}
}
void Parser::Simulate(ShiftReduceModel & model, const FeatureSet & feature_gen,
		const ActionVector & actions, const Sentence & sent,
		const int firstdiff, FeatureMapInt & featmap, double c) {
	int n = sent[0]->GetNumWords();
	DPStateVector stateseq;
	DPState * state = InitialState();
	stateseq.push_back(state);
	if (verbose_ >= 2){
		cerr << "Simulate actions:";
		for (int step = 0 ; step < actions.size() ; step++)
			cerr << " " << (char) actions[step];
		cerr << endl;
	}

	int i = 1;
	BOOST_FOREACH(DPState::Action action, actions){
		if (verbose_ >= 2)
			cerr << *state << endl;
		if (i++ >= firstdiff){
			const FeatureVectorInt * fvi = feature_gen.MakeStateFeatures(
					sent, *state, action, model.GetFeatureIds(), model.GetAdd());
			if (verbose_ >= 2){
				FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
				BOOST_FOREACH(FeaturePairString feat, *fvs)
					cerr << feat.first << ":" << feat.second << " ";
				cerr << endl;
				delete fvs;
			}
			BOOST_FOREACH(FeaturePairInt feat, *fvi){
				FeatureMapInt::iterator it = featmap.find(feat.first);
				if (it == featmap.end())
					featmap[feat.first] = 0;
				featmap[feat.first] += c * feat.second;
			}
			delete fvi;
		}
		if (state->Allow(action, n)){
			state->Take(action, stateseq); // only one item
			state = stateseq.back();
		}
		else{
			state->PrintTrace(cerr);
			THROW_ERROR("Bad action! " << (char) action << endl);
		}
	}
	// clean-up states
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
}
} /* namespace lader */
