/*
 * parser.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <shift-reduce-dp/ddpstate.h>
#include <shift-reduce-dp/shift-reduce-model.h>
#include <lader/feature-data-base.h>
#include <lader/feature-vector.h>
#include <lader/loss-base.h>
#include <boost/foreach.hpp>
#include <string>
#include <vector>
using namespace std;
namespace lader {

class Parser {
public:
	Parser();
	virtual ~Parser();
	void Clear();
	typedef struct {
		vector<int> order;
		ActionVector actions;
		double score;
		int step;
	} Result;

	virtual DPState * InitialState() { return new DPState(); }
	DPState * GuidedSearch(const ActionVector & refseq, int n);
	virtual void Search(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			const Ranks * ranks = NULL);
	// latent variable version
	void Update(Result * result, const string * update);
	void GetKbestResult(vector<Parser::Result> & kbest);
	void Simulate(ShiftReduceModel & model, const FeatureSet & feature_gen,
			const ActionVector & actions, const Sentence & sent,
			const int firstdiff, std::tr1::unordered_map<int,double> & featmap, double c);
	void SetBeamSize(int beamsize) { beamsize_ = beamsize; }
	void SetVerbose(int verbose) { verbose_ = verbose; }

	int GetNumStates() const { return nstates_; }
	int GetNumEdges() const { return nedges_; }
	int GetNumUniq() const { return nuniq_; }
	int GetNumGolds() const {
		unsigned long count = 0;
		BOOST_FOREACH(DPState * gold, golds_.back())
			count += gold->GetNumDerivations();
		return count;
	}
	int GetNumParses() const {
		int k;
		for (k = 0 ; GetKthBest(k) != NULL ; k++)
			;
		return k;
	}
	DPState * GetBest() const { return GetKthBest(0); }
	DPState * GetKthBest(int k) const {
		if (beams_.empty())
			THROW_ERROR("Search result is empty!")
		if (k >= beams_.back().size())
			return NULL;
		return beams_.back()[k];
	}
	DPState * GetBeamBest(int step) const {
		const DPStateVector & b = SafeAccess(beams_, step);
		if (b.empty())
			return NULL;
		return b[0];
	}
	DPState * GetGoldBest(int step) const {
		const DPStateVector & b = SafeAccess(golds_, step);
		if (b.empty())
			return NULL;
		return b[0];
	}
	static void SetResult(Result & result, const DPState * goal);

    // Add up the loss over an entire sentence
    void AddLoss(
    		LossBase* loss,
    		const Ranks * ranks,
            const FeatureDataParse * parse) ;
    // Rescore a state
    double Rescore(double loss_multiplier, DPState * state);
    // Rescore the hypergraph using the given model and a loss multiplier
    double Rescore(double loss_multiplier);
    // Add up the loss over an entire subtree defined by a state
    double AccumulateLoss(const DPState* state);
    void AccumulateFeatures(FeatureMapInt & featmap,
    									ReordererModel & model,
    									const FeatureSet & features,
    									const Sentence & sent,
    									const DPState * goal);

protected:
	virtual bool Allow(DPState * state, DPState::Action action, int n) {
		return state->Allow(action, n);
	}
	virtual bool IsGold(DPState::Action action, const Ranks * ranks,
			DPState * old) const;
	// latent variable version
	void DynamicProgramming(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			const Ranks * ranks = NULL);
	vector< DPStateVector > beams_;		// store dp states produced by decoding
	vector< DPStateVector > golds_;		// store all possible gold states, assuming latent variable
	int beamsize_;						// the maximum # states in a bin
	int nstates_, nedges_, nuniq_;		// # of produced states, edges, and unique states
	int verbose_;						// control verbosity
	ActionVector actions_;				// all possible actions for this parser
};


} /* namespace lader */

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::Parser::Result & rhs )
{
    out << "Step " << rhs.step << ": " << rhs.score << " ::";
    BOOST_FOREACH(lader::DPState::Action action, rhs.actions)
    	out << " " << (char)action;
    out << " ::";
    BOOST_FOREACH(int i, rhs.order)
        out << " " << i;
    return out;
}
}
#endif /* PARSER_H_ */
