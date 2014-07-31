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
//	virtual void Search(ShiftReduceModel & model,
//			const FeatureSet & feature_gen, const Sentence & sent,
//			Result * result = NULL, const ActionVector * refseq = NULL,
//			const string * update = NULL) {
//		DPStateVector golds(2*(sent[0]->GetNumWords() + maxins_), NULL);
//		DynamicProgramming(golds, model, feature_gen, sent, refseq);
//		if (result && refseq){
//			DPStateVector simgolds;
//			CompleteGolds(simgolds, golds, model, feature_gen, sent, refseq);
//			Update(golds, result, refseq, update);
//			// clean-up the rest gold items using refseq
//			BOOST_FOREACH(DPState * gold, simgolds)
//				delete gold;
//		}
//	}

	virtual void Search(ShiftReduceModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			Result * result = NULL, const Ranks * ranks = NULL,
			const string * update = NULL) {
		Clear();
		golds_.resize(2*sent[0]->GetNumWords() + maxins_, DPStateVector());
		DynamicProgramming(model, feature_gen, sent, ranks);
		if (result && update)
			Update(result, update);
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

//	virtual bool IsGold(DPState::Action action, const Ranks * ranks,
//			DPState * old) const {
//		return Parser::IsGold(action, ranks, old);
////		|| (old->Allow(DPState::INSERT_L, n) && !IsDeleted(cal, old) && lstate->GetAction() != DPState::INIT && ranks_[lstate->GetSrcL()] == Ranks::INSERT_L)
////		|| (old->Allow(DPState::INSERT_R, n) && !IsDeleted(cal, old) && old->GetSrcREnd() < ranks_.size() && ranks_[old->GetSrcREnd()] == Ranks::INSERT_R)
////		|| (old->Allow(DPState::DELETE_L, n) && IsDeleted(cal, lstate) && !IsDeleted(cal, old) && HasTie(lstate))
////		|| (old->Allow(DPState::DELETE_R, n) && !IsDeleted(cal, lstate) && IsDeleted(cal, old) && HasTie(lstate));
//	}
private:
	int maxins_, maxdel_;		// the maximum number of insert/delete actions
};


} /* namespace lader */



#endif /* IPARSER_H_ */
