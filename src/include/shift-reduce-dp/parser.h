/*
 * parser.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <shift-reduce-dp/dpstate.h>
#include <lader/feature-data-base.h>
#include <lader/feature-vector.h>
#include <string>
#include <vector>
using namespace std;
namespace lader {

class Parser {
public:
	Parser();
	virtual ~Parser();
	struct Result{
		vector<int> order;
		vector<DPState::Action> actions;
		double score;
		int step;
	};

	void Search(ReordererModel & model,
	        const FeatureSet & feature_gen,
	        const Sentence & sent, Result & result,
			vector<DPState::Action> * refseq = NULL, string * update = NULL);
	void Simulate(ReordererModel & model, const FeatureSet & feature_gen,
			const vector<DPState::Action> & actions, const Sentence & sent,
			const int firstdiff, std::tr1::unordered_map<int,double> & featmap, double c);
	void SetBeamSize(int beamsize) { beamsize_ = beamsize; }
	void SetVerbose(int verbose) { verbose_ = verbose; }

	int GetNumStates() const { return nstates_; }
	int GetNumEdges() const { return nedges_; }
private:
	void Update(vector< DPStateVector > & beams, DPStateVector & golds,
			Result & result, vector<DPState::Action> * refseq = NULL,
			string * update = NULL);
	int beamsize_;
	int nstates_, nedges_, nuniq_;
	int verbose_;
};

} /* namespace lader */
#endif /* PARSER_H_ */
