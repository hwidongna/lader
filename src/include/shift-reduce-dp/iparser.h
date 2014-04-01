/*
 * iparser.h
 *
 *  Created on: Mar 31, 2014
 *      Author: leona
 */

#ifndef IPARSER_H_
#define IPARSER_H_

#include <shift-reduce-dp/parser.h>
#include <shift-reduce-dp/idpstate.h>

using namespace std;

namespace lader {

class IParser : public Parser{
public:
	IParser(int maxins, int maxdel) : maxins_(maxins), maxdel_(maxdel) {
		actions_.push_back(DPState::IDLE);
		actions_.push_back(DPState::INSERT_L);
		actions_.push_back(DPState::INSERT_R);
		actions_.push_back(DPState::DELETE_L);
		actions_.push_back(DPState::DELETE_R);
	}
	virtual ~IParser() { }
	virtual DPState * InitialState() { return new IDPState(); }
	virtual void Search(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			Result * result = NULL, const ActionVector * refseq = NULL,
			const string * update = NULL) {
		DPStateVector golds(2*(sent[0]->GetNumWords() + maxins_), NULL);
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
protected:
	virtual bool Allow(DPState * state, DPState::Action action, int n) {
		IDPState * iold = dynamic_cast<IDPState*>(state);
		if (!iold)
			THROW_ERROR("Cannot cast to IDPState: " << *state << endl)
		return state->Allow(action, n) &&
				((action != DPState::INSERT_L && action != DPState::INSERT_R &&	action != DPState::DELETE_L && action != DPState::DELETE_R) ||
				((action == DPState::INSERT_L || action == DPState::INSERT_R) && iold->GetNumIns() < maxins_) ||
				((action == DPState::DELETE_L || action == DPState::DELETE_R) && iold->GetNumDel() < maxdel_));
	}
private:
	int maxins_, maxdel_;		// the maximum number of insert/delete actions
};


} /* namespace lader */



#endif /* IPARSER_H_ */
