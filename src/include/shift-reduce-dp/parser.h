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
#include <boost/foreach.hpp>
#include <string>
#include <vector>
using namespace std;
namespace lader {

class Parser {
public:
	Parser();
	virtual ~Parser();
	typedef struct {
		vector<int> order;
		vector<DPState::Action> actions;
		double score;
		int step;
	} Result;

	virtual DPState * InitialState() { return new DPState(); }
	DPState * GuidedSearch(const vector<DPState::Action> & refseq, int n);
	virtual void Search(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			Result * result = NULL, const vector<DPState::Action> * refseq = NULL,
			const string * update = NULL);
	void GetKbestResult(vector<Parser::Result> & kbest);
	void Simulate(ShiftReduceModel & model, const FeatureSet & feature_gen,
			const vector<DPState::Action> & actions, const Sentence & sent,
			const int firstdiff, std::tr1::unordered_map<int,double> & featmap, double c);
	void SetBeamSize(int beamsize) { beamsize_ = beamsize; }
	void SetVerbose(int verbose) { verbose_ = verbose; }

	int GetNumStates() const { return nstates_; }
	int GetNumEdges() const { return nedges_; }
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
protected:
	virtual bool Allow(DPState * old, DPState::Action action, int n) {
		return old->Allow(action, n);
	}
	void DynamicProgramming(DPStateVector & golds, ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			const vector<DPState::Action> * refseq = NULL);
	void CompleteGolds(DPStateVector & simgolds, DPStateVector & golds,
			ShiftReduceModel & model, const FeatureSet & feature_gen,
			const Sentence & sent, const vector<DPState::Action> * refseq);
	void Update(DPStateVector & golds, Result * result,
			const vector<DPState::Action> * refseq, const string * update);
	vector< DPStateVector > beams_;
	int beamsize_;
	int nstates_, nedges_, nuniq_;
	int verbose_;
	vector<DPState::Action> actions_;
};
void SetResult(Parser::Result * result, const DPState * goal);


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
