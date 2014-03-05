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

	virtual void Take(Action action, DPStateVector & result, bool actiongold = false,
			int maxterm = 1, ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	virtual bool Allow(const Action & action, const int n);
	virtual bool IsContinuous() { return src_l2_ < 0 && src_r2_ < 0; }
	int GetSrcL2() const { return src_l2_; }
	int GetSrcR2() const { return src_r2_; }
	int GetSrcREnd() const { return src_rend_; }
	DPStateVector GetSwaped() const { return swaped_; }
protected:
	virtual DPState * Shift();
	virtual DPState * Reduce(DPState * leftstate, Action action);
	DPState * Swap(DPState * leftstate);
	int src_l2_, src_r2_;	// discontinuous source span
	int src_rend_;			// source index for next shift
	DPStateVector swaped_;	// swaped states
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
		<< " swap " << rhs.GetSwaped().size();
    return out;
}
}
#endif /* DDPSTATE_H_ */
