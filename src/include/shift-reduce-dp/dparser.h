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
	DParser(int m=1) : maxswap_(m) {
		actions_.push_back(DPState::IDLE);
		actions_.push_back(DPState::SWAP);
	}
	virtual ~DParser() { }
	virtual DPState * InitialState() { return new DDPState(); }
//	virtual void Search(ShiftReduceModel & model,
//			const FeatureSet & feature_gen, const Sentence & sent,
//			Result * result = NULL, const ActionVector * refseq = NULL,
//			const string * update = NULL) {
//		DPStateVector golds(2*(sent[0]->GetNumWords() + m_), NULL);
//		DynamicProgramming(golds, model, feature_gen, sent, refseq);
//		if (!GetBest()){ // SWAP may result parsing fail with limited beam size
//			Parser::Clear();
//			actions_.pop_back(); // try ITG parsing instead
//			golds.clear();
//			golds.resize(2*(sent[0]->GetNumWords() + m_), NULL);
//			DynamicProgramming(golds, model, feature_gen, sent, refseq);
//		}
//		if (result && refseq && update){
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
		golds_.resize(2*(sent[0]->GetNumWords() + maxswap_), DPStateVector());
		DynamicProgramming(model, feature_gen, sent, ranks);
		if (result && update)
			Update(result, update);
	}
protected:
	virtual bool Allow(DPState * state, DPState::Action action, int n) {
		DDPState * dold = dynamic_cast<DDPState*>(state);
		if (!dold)
			THROW_ERROR("Cannot cast to DDPState: " << *state << endl)
		return state->Allow(action, n) && (action != DPState::SWAP || dold->GetNumSwap() < maxswap_);
	}

	virtual bool IsGold(DPState::Action action, const Ranks * ranks,
			DPState * old) const {
		DPState * leftstate = old->GetLeftState();
		return Parser::IsGold(action, ranks, old)
		|| (action == DPState::SWAP && ranks->IsSwap(old)
				&& !(ranks->IsStraight(leftstate, old)
				|| (ranks->IsInverted(leftstate, old) && !ranks->HasTie(old))));
	}

private:
	int maxswap_;		// the maximum number of swap actions
};


} /* namespace lader */


#endif /* DPARSER_H_ */
