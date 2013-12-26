/*
 * parser.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/parser.h"
#include <boost/foreach.hpp>


namespace lader {

Parser::Parser(): nstates_(0), nedges_(0), nuniq_(0)  {

}

Parser::~Parser() {
}

void Parser::Search(const Sentence & sent, Result & result,
		vector<DPState::Action> * refseq, string * update){
	int n = sent[0]->GetNumWords();
	int maxStep = 2*n;
	vector< DPStateVector > beams(maxStep, DPStateVector());
	DPState * state = new DPState();
	beams[0].push_back(state); // initial state

	DPStateVector golds(maxStep, NULL);
	golds[0] = beams[0][0]; // initial state
	if (verbose_ >= 2)
		BOOST_FOREACH(DPState * gold, golds)
			if (gold != NULL)
				cerr << *gold << endl;
	for (int step = 1 ; step < maxStep ; step++){
		if (verbose_ >= 2)
			cerr << "step " << step << endl;
		DPStateQueue q;
		BOOST_FOREACH(DPState * old, beams[step-1]){
			if (verbose_ >= 2)
				cerr << "OLD: " << *old << endl;
			// iterate over actions by abusing enum type
			for (int action = DPState::SHIFT ; action < DPState::NOP ; action++){
				if (!old->Allow((DPState::Action)action, n))
					continue;
				bool actiongold = (refseq != NULL && action == (*refseq)[step-1]);
				BOOST_FOREACH(DPState * next, old->Take((DPState::Action)action, actiongold)){
					if (next->IsGold())
						golds[step] = next;
					q.push(next);
					if (verbose_ >= 2)
						cerr << "NEW: " << *next << endl;
				}
			}
		}
		nedges_ += q.size(); // number of created states (correlates with running time)
		if (verbose_ >= 2)
			cerr << "produce " << q.size() << " items" << endl;

		DPStateMap tmp;
		int rank = 0;
		for (int k = 0 ; !q.empty() && k < beamsize_ ; k++){
			DPState * top = q.top(); q.pop();
			DPStateMap::iterator it = tmp.find(*top);
			if (it == tmp.end()){
				tmp[*top] = top;
				beams[step].push_back(top);
				top->SetRank(rank++);
				if (verbose_ >= 2)
					cerr << *top << endl;
			}
			else{
				it->second->MergeWith(top);
				if (top->IsGold()){
					if (verbose_ >= 2)
						cerr << "GOLD at " << k << " MERGED with " << it->second->GetRank() << endl;
				}
				else
					delete top;
			}
		}
		nstates_ += beams[step].size(); // number of survived states
		if (verbose_ >= 2)
			cerr << "survive " << beams[step].size() << " items" << endl;
		nuniq_ += rank;
		// clean-up the rest
		while (!q.empty()){
			delete q.top(); q.pop();
		}
	}

	Update(beams, golds, result, refseq, update);
	// TODO: clean-up beams
	for (int step = 0 ; step < maxStep ; step++)
		BOOST_FOREACH(DPState * state, beams[step])
			delete state;
}

// support naive, early, max update for perceptron
void Parser::Update(vector< DPStateVector > & beams, DPStateVector & golds,
		Result & result, vector<DPState::Action> * refseq, string * update) {
	if (update != NULL and *update != "naive"){
		if (verbose_ >= 2){
			cerr << "update strategy: " << *update << endl;
			cerr << "gold size: " << golds.size() << ", beam size: " << beams.size() << endl;
		}
		int earlypos, latepos, largepos, maxpos;
		double maxdiff = 0;
		if (golds.size() != beams.size())
			THROW_ERROR("gold size " << golds.size() << " != beam size" << beams.size() << endl);
		if (verbose_ >= 2)
			BOOST_FOREACH(DPState * gold, golds)
				if (gold != NULL)
					cerr << *gold << endl;
		DPStateVector goldseq;
		for (int step = 1 ; step < (int)beams.size() ; step++){
			if (golds[step] == NULL){
				goldseq.push_back(golds[step-1]->Take((*refseq)[step-1], true)[0]); // only one next item
				golds[step] = goldseq.back();
				if (verbose_ >= 2)
					cerr << "SIMGOLD: " << *golds[step] << endl;
			}

			DPState * best = beams[step][0]; // update against best
			if (verbose_ >= 2){
				cerr << "BEST: " << *best << endl;
				cerr << "GOLD: " << *golds[step] << endl;
			}
			if (golds[step]->GetRank() < 0){ // fall-off
				if (*update == "early"){
					best->AllActions(result.actions);
					result.step = step;
					break;
				}
				else if (*update == "max"){
					double mdiff = best->GetScore() - golds[step]->GetScore();
					if (mdiff >= maxdiff){
						maxdiff = mdiff;
						maxpos = step;
					}
				}
			}
		}
		BOOST_FOREACH(DPState * gold, goldseq)
			delete gold;
		if (*update == "max"){
			beams[maxpos][0]->AllActions(result.actions);
			result.step = maxpos;
		}
	}
	else{
		DPState * goal = beams[beams.size()-1][0];
		goal->GetReordering(result.order);
		result.step = goal->GetStep();
		goal->AllActions(result.actions);
		result.score = goal->GetScore();
	}
}

void Parser::Simulate(const vector<DPState::Action> & actions, const Sentence & sent,
			const int firstdiff, FeatureVectorInt & actionfeats, int c){
	int n = sent[0]->GetNumWords();
	DPStateVector stateseq;
	DPState * state = new DPState();
	stateseq.push_back(state);
	for (int step = 1 ; step < (const int)actions.size() ; step++){
		if (step >= firstdiff){
			// TODO: generate features
			// TODO: +c to actionfeats
		}
		if (state->Allow(actions[step], n)){
			stateseq.push_back(state->Take(actions[step])[0]); // only one item
			state = stateseq.back();
		}
		else{
			// bad sequence
			break;
		}
	}
	// clean-up states
	BOOST_FOREACH(DPState * state, stateseq)
		delete state;
}
} /* namespace lader */
