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
#include <float.h>

using namespace std;

namespace lader {

Parser::Parser(): nstates_(0), nedges_(0), nuniq_(0), beamsize_(0), verbose_(0)  {
	actions_.push_back(DPState::SHIFT);
	actions_.push_back(DPState::STRAIGTH);
	actions_.push_back(DPState::INVERTED);
}

Parser::~Parser() {
	Clear();
}

void Parser::Clear() {
	BOOST_FOREACH(DPStateVector & b, beams_)
		BOOST_FOREACH(DPState * state, b){
			if (!state->IsGold())
				delete state;
		}
	beams_.clear();
	BOOST_FOREACH(DPStateVector & b, golds_)
		BOOST_FOREACH(DPState * state, b)
			delete state;
	golds_.clear();
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
		const Ranks * ranks) {
	Clear();
	golds_.resize(2*sent[0]->GetNumWords(), DPStateVector());
	DynamicProgramming(model, feature_gen, sent, ranks);
}

bool Parser::IsGold(DPState::Action action, const Ranks * ranks,
		DPState * old) const {
	DPState * leftstate = old->GetLeftState();
	switch(action){
	case DPState::STRAIGTH:
		return ranks->IsStraight(leftstate, old);
	case DPState::INVERTED:
		return ranks->IsInverted(leftstate, old) && !ranks->HasTie(old);
	case DPState::SHIFT: // if not reduce
		return !(ranks->IsStraight(leftstate, old)
				|| (ranks->IsInverted(leftstate, old) && !ranks->HasTie(old)))
				// or possible to reduce after shift
				|| Ranks::IsContiguous((*ranks)[old->GetTrgR()-1], (*ranks)[old->GetSrcREnd()])
				|| Ranks::IsStepOneUp((*ranks)[old->GetSrcREnd()], (*ranks)[old->GetTrgR()-1]);
	case DPState::IDLE:
		return true;
	default:
		return false;
	}
}
void Parser::DynamicProgramming(ShiftReduceModel & model,
		const FeatureSet & feature_gen, const Sentence & sent,
		const Ranks * ranks) {
	int n = sent[0]->GetNumWords();
	int maxstep = golds_.size();
	beams_.resize(golds_.size(), DPStateVector());
	DPState * state = InitialState();
	beams_[0].push_back(state); // initial state
	golds_[0].push_back(state);
	for (int step = 1 ; step < maxstep ; step++){
		if (verbose_ >= 2)
			cerr << "step " << step << endl;
		DPStateQueue q;
		BOOST_FOREACH(DPState * old, beams_[step-1]){
			if (verbose_ >= 2){
				cerr << "OLD: "; old->Print(cerr); cerr << endl;
			}
			// iterate over actions
			BOOST_FOREACH(DPState::Action action, actions_){
				if (!Allow(old, action, n))
					continue;
				bool actiongold = (ranks != NULL && IsGold(action, ranks, old));
				DPStateVector stateseq;
				old->Take(action, stateseq, actiongold, 1,
						&model, &feature_gen, &sent);
				BOOST_FOREACH(DPState * next, stateseq){
					next->SetSignature(model.GetMaxState());
					q.push(next);
					if (verbose_ >= 2){
						cerr << "  NEW: "; next->Print(cerr); cerr << endl;
					}
				}
			}
		}
		// complete gold states if falled-off
		BOOST_FOREACH(DPState * old, golds_[step-1]){
			if (old->GetRank() >= 0) // fall-in cases
				continue;
			if (verbose_ >= 2){
				cerr << "OLD: "; old->Print(cerr); cerr << endl;
			}
			DPState * leftstate = old->GetLeftState();
			// iterate over actions
			BOOST_FOREACH(DPState::Action action, actions_){
				if (!Allow(old, action, n))
					continue;
				bool actiongold = (ranks != NULL && IsGold(action, ranks, old));
				DPStateVector stateseq;
				old->Take(action, stateseq, actiongold, 1,
						&model, &feature_gen, &sent);
				BOOST_FOREACH(DPState * next, stateseq){
					next->SetSignature(model.GetMaxState());
					q.push(next);
					if (verbose_ >= 2){
						cerr << "  NEW: "; next->Print(cerr); cerr << endl;
					}
				}
			}
		}
		nedges_ += q.size(); // total number of created states (correlates with running time)
		if (verbose_ >= 2)
			cerr << "produce " << q.size() << " items" << endl;

		DPStateMap tmp;
		int rank = 0;
		for (int k = 0 ; !q.empty() && (!beamsize_ || k < beamsize_) ; k++){
			DPState * top = q.top(); q.pop();
			DPStateMap::iterator it = tmp.find(top); // utilize hash() and operator==()
			if (top->IsGold())
				golds_[step].push_back(top);
			if (it == tmp.end()){
				tmp[top] = top;
				beams_[step].push_back(top);
				top->SetRank(rank++);
				if (verbose_ >= 2){
					top->Print(cerr); cerr << endl;
				}
			}
			else{
				it->second->MergeWith(top);
				if (top->IsGold()){ // fall-off by merge
					if (verbose_ >= 2)
						cerr << "GOLD at " << k << " MERGED with " << it->second->GetRank() << endl;
				}
				else	// do not delete gold state, will be deleted when Clear
					delete top;
			}
		}
		nstates_ += beams_[step].size(); 	// total number of survived states
		if (verbose_ >= 2)
			cerr << "survive " << beams_[step].size() << " items" << endl;
		nuniq_ += rank;						// total number of uniq states
		// clean-up the rest
		DPStateMap gmap;
		BOOST_FOREACH(DPState * top, golds_[step])
			gmap[top] = top;
		while (!q.empty()){
			DPState * top = q.top(); q.pop();
			DPStateMap::iterator it = gmap.find(top); // utilize hash() and operator==()
			if (top->IsGold() && golds_[step].size() < beamsize_){
				if (it == tmp.end()){
					gmap[top] = top;
					golds_[step].push_back(top); // fall-off by beam search
				}
				else{
					it->second->MergeWith(top);
					if (verbose_ >= 2)
						cerr << "GOLD " << *top << " MERGED with " << it->second->GetRank() << endl;
					delete top;
				}
			}
			else
				delete top;
		}
		if (ranks){ // if trainning
			if (golds_[step].empty()){ // unreachable trainning instance
				if (verbose_ >= 1)
					cerr << "Fail to produce any gold derivation at step " << step << endl;
//				break; // stop decoding
			}
			if (verbose_ >= 2)
				cerr << "survive " << golds_[step].size() << " golds" << endl;
		}
	}
}

void Parser::Update(Result * result, const string * update) {
	int earlypos = -1, latepos, largepos, maxpos = -1, naivepos;
	if (verbose_ >= 2){
		cerr << "update strategy: " << *update << endl;
		cerr << "gold size: " << golds_.size() << ", beam size: " << beams_.size() << endl;
	}
	double maxdiff = 0;
	if (golds_.size() != beams_.size())
		THROW_ERROR("gold size " << golds_.size() << " != beam size" << beams_.size() << endl);
	for (int step = 1 ; step < (int)beams_.size() ; step++){
		const DPState * best = GetBeamBest(step); // update against best
		const DPState * gold = GetGoldBest(step); // update against best
		if (!best){
			BOOST_FOREACH(DPState * prev, beams_[step-1]){
				prev->Print(cerr); cerr << endl;
			}
			THROW_ERROR("Parsing failure at step " << step << endl);
		}
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
	SetResult(*result, beams_[naivepos][0]);
}

void Parser::SetResult(Parser::Result & result, const DPState * goal){
	if (!goal)
		THROW_ERROR("SetResult does not accept NULL arguments " << endl)
	goal->GetReordering(result.order);
	result.step = goal->GetStep();
	goal->AllActions(result.actions);
	result.score = goal->GetScore();
}

void Parser::GetKbestResult(vector<Result> & kbest){
	for (int k = 0 ; GetKthBest(k) != NULL ; k++){
		Result result;
		SetResult(result, GetKthBest(k));
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
			cerr << "Simulate fails:";
			BOOST_FOREACH(string word, sent[0]->GetSequence())
				cerr << " " << word;
			cerr << endl;
			state->PrintTrace(cerr);
			THROW_ERROR("Bad action! " << (char) action << endl);
		}
	}
	// clean-up states
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
}


// Score a state
double Parser::Rescore(double loss_multiplier, DPState * state) {
    double score = state->GetRescore();
    if(score == -DBL_MAX) {
        score = state->GetLoss()*loss_multiplier;
        if(state->LeftChild())
            score += Rescore(loss_multiplier, state->LeftChild());
        if(state->RightChild())
			score += Rescore(loss_multiplier, state->RightChild());
		score += state->GetActionCost();
        state->SetRescore(score);
    }
    return score;
}


template <class T>
struct DescendingRescore {
  bool operator ()(T *lhs, T *rhs) { return rhs->GetRescore() < lhs->GetRescore(); }
};

double Parser::Rescore(double loss_multiplier) {
	// Reset everything to -DBL_MAX to indicate it needs to be recalculated
	for (int i = 0 ; i < beams_.size() ; i++)
		BOOST_FOREACH(DPState * state, beams_[i])
			state->SetRescore(-DBL_MAX);
	// Recursively score all states from the root
	BOOST_FOREACH(DPState * state, beams_.back())
		Rescore(loss_multiplier, state);
	// Sort to make sure that the spans are all in the right order
	for (int i = 0 ; i < beams_.size() ; i++)
		sort(beams_[i].begin(), beams_[i].end(), DescendingRescore<DPState>());
    DPState * best = GetBest();
    return best->GetRescore();
}

// Add up the loss over an entire subtree defined by state
double Parser::AccumulateLoss(const DPState* state) {
    double score = state->GetLoss();
    if(state->LeftChild())  score += AccumulateLoss(state->LeftChild());
    if(state->RightChild())  score += AccumulateLoss(state->RightChild());
    return score;
}

void Parser::AddLoss(LossBase* loss,
		const Ranks * ranks, const FeatureDataParse * parse) {
    // Initialize the loss
    loss->Initialize(ranks, parse);
    // For each span in the hypergraph
    for (int i = 0 ; i < beams_.size() ; i++)
    	BOOST_FOREACH(DPState * state, beams_[i])
    		state->SetLoss(state->GetLoss()
    				+ loss->GetStateLoss(state, i == beams_.size()-1, ranks, parse));
}

void Parser::AccumulateFeatures(FeatureMapInt & featmap,
									ReordererModel & model,
									const FeatureSet & features,
									const Sentence & sent,
									const DPState * goal){
	ActionVector steps;
	goal->AllActions(steps);
	int n = sent[0]->GetNumWords();
	DPStateVector stateseq;
	DPState * state = InitialState();
	stateseq.push_back(state);
	if (verbose_ >= 2)
		cerr << "/************************************************************************************************/" << endl;
	for (int i = 0 ; i < steps.size() ; i++){
		DPState::Action & action = steps[i];
		if (verbose_ >= 2)
			cerr << *state << endl;
		const FeatureVectorInt * fvi = features.MakeStateFeatures(
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
			featmap[feat.first] += feat.second;
		}
		delete fvi;
		if (state->Allow(action, n)){
			state->Take(action, stateseq); // only one item
			state = stateseq.back();
		}
		else{
			cerr << "Simulate fails:";
			BOOST_FOREACH(string word, sent[0]->GetSequence())
			cerr << " " << word;
			cerr << endl;
			state->PrintTrace(cerr);
			THROW_ERROR("Bad action! " << (char) action << endl);
		}
	}
	if (verbose_ >= 2)
		cerr << "/************************************************************************************************/" << endl;
	// clean-up states
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
}

} /* namespace lader */
