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
		vector<DPState::Action> actions;
		double score;
		int step;
	} Result;

	void Search(ReordererModel & model, const FeatureSet & feature_gen,
			const Sentence & sent, Result & result, int max_state = 0,
			vector<DPState::Action> * refseq = NULL, string * update = NULL);
	void Simulate(ReordererModel & model, const FeatureSet & feature_gen,
			const vector<DPState::Action> & actions, const Sentence & sent,
			const int firstdiff, std::tr1::unordered_map<int,double> & featmap, double c);
	void SetBeamSize(int beamsize) { beamsize_ = beamsize; }
	void SetVerbose(int verbose) { verbose_ = verbose; }

	int GetNumStates() const { return nstates_; }
	int GetNumEdges() const { return nedges_; }
	DPState * GetBest() const {
		if (beams_.empty() || beams_.back().empty())
			THROW_ERROR("Search result is empty!")
		return beams_.back()[0];
	}
private:
	void Update(vector< DPStateVector > & beams, DPStateVector & golds,
			Result & result, vector<DPState::Action> * refseq = NULL,
			string * update = NULL);
	vector< DPStateVector > beams_;
	int beamsize_;
	int nstates_, nedges_, nuniq_;
	int verbose_;
};


} /* namespace lader */

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::Parser::Result & rhs )
{
    out << rhs.step << ": " << rhs.score << " ::";
    BOOST_FOREACH(lader::DPState::Action action, rhs.actions)
    	out << " " << action;
    out << endl;
    return out;
}
}
#endif /* PARSER_H_ */
