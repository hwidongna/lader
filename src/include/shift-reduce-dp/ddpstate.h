/*
 * ddpstate.h
 *
 *  Created on: Mar 4, 2014
 *      Author: leona
 */

#ifndef DDPSTATE_H_
#define DDPSTATE_H_

#include <shift-reduce-dp/dpstate.h>
using namespace std;

namespace lader{

class DDPState : public DPState{
public:

	DDPState(); // for initial state
	virtual ~DDPState();
	DDPState(int step, int i, int j, Action action, int i2=-1, int j2=-1);
	virtual void MergeWith(DPState * other);
	virtual void Take(Action action, DPStateVector & result, bool actiongold = false,
			int maxterm = 1, ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	virtual bool Allow(const Action & action, const int n);
	virtual void InsideActions(vector<Action> & result) const;
	virtual DPState * LeftChild() const;
	virtual DPState * RightChild() const;
	virtual DPState * Previous() const;
	virtual void GetReordering(vector <int> & result) const;
	virtual bool IsContinuous() { return src_l2_ < 0 && src_r2_ < 0; }
	virtual int GetSrcREnd() const { return src_rend_; }
	int GetSrcL2() const { return src_l2_; }
	int GetSrcR2() const { return src_r2_; }
	DPStateVector GetSwaped() const { return swapped_; }
	int GetNumSwap() const { return nswap_; }

	// compare signature
	virtual bool operator == (const DPState & other) const {
		if (GetAction() == DPState::SWAP){
			const DDPState * dstate = dynamic_cast<const DDPState*>(&other);
			if (!dstate || swapped_.size() != dstate->swapped_.size() || dstate->GetAction() != DPState::SWAP)
				return false;
			return std::equal(swapped_.begin(), swapped_.end(), dstate->swapped_.begin())
			&& src_l2_ == dstate->src_l2_ && DPState::operator ==(other);
		}
		return DPState::operator ==(other);
	}
	virtual void PrintParse(const vector<string> & strs, ostream & out) const;
	virtual void Print(ostream & out) const;
	virtual DPStateNode * ToFlatTree();
protected:
	virtual DPState * Shift();
	virtual DPState * Reduce(DPState * leftstate, Action action);
	virtual DPState * Swap(DPState * leftstate);
	virtual DPState * Idle();
	int src_l2_, src_r2_;	// discontinuous source span
	int src_rend_;			// source index for the next buffer front
	DPStateVector swapped_;	// swaped states
	DPState * trace_;		// the actual state, if this is a swaped one
	int nswap_;				// the number of swap actions taken so far
};

}

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::DDPState & rhs )
{
    out << (rhs.IsGold() ? "*" : " ") << rhs.GetStep() << "("
    	<< (char)rhs.GetAction() << ", " << rhs.GetRank() << "):"
    	<< "< " << rhs.GetSrcL() << ", " << rhs.GetSrcR()-1 << ", "
    	<< rhs.GetSrcL2() << ", " << rhs.GetSrcR2()-1 << ", "
    	<< rhs.GetSrcREnd() << ", "
		<< rhs.GetTrgL() << ", " << rhs.GetTrgR()-1 << " > :: "
		<< rhs.GetScore() << ", " << rhs.GetInside()
		<< ": left " << rhs.GetLeftPtrs().size()
		<< ", swap " << rhs.GetSwaped().size()
		<< ", nswap " << rhs.GetNumSwap();
    return out;
}
}
#endif /* DDPSTATE_H_ */
