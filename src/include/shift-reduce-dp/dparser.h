/*
 * dparser.h
 *
 *  Created on: Mar 5, 2014
 *      Author: leona
 */

#ifndef DPARSER_H_
#define DPARSER_H_

#include <shift-reduce-dp/parser.h>
#include <shift-reduce-dp/ddpstate.h>

using namespace std;

namespace lader {

class DParser : public Parser{
public:
	DParser(int m=1) : m_(m) {
		actions_.push_back(DPState::SWAP);
		actions_.push_back(DPState::IDLE);
	}
	virtual ~DParser() { }
	virtual DPState * InitialState() { return new DDPState(); }
	virtual void Search(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			Result * result = NULL, const vector<DPState::Action> * refseq = NULL,
			const string * update = NULL) {
		DPStateVector golds(2*(sent[0]->GetNumWords() + m_), NULL);
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
private:
	int m_;		// the maximum number of swap actions
};


} /* namespace lader */


#endif /* DPARSER_H_ */
