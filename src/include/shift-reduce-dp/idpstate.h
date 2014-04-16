/*
 * idpstate.h
 *
 *  Created on: Mar 31, 2014
 *      Author: leona
 */

#ifndef IDPSTATE_H_
#define IDPSTATE_H_

#include <shift-reduce-dp/dpstate.h>
#include <vector>
using namespace std;

namespace lader{

// A dynamic programming state for an intermediate tree
// supports insert/delete actions for reduce the difference
// between the source and target languages.
class IDPState : public DPState{
public:

	IDPState(); // for initial state
	virtual ~IDPState();
	IDPState(int step, int i, int j, Action action);
	virtual void MergeWith(DPState * other);
	virtual void Take(Action action, DPStateVector & result, bool actiongold = false,
			int maxterm = 1, ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	virtual bool Allow(const Action & action, const int n);
	virtual void InsideActions(ActionVector & result) const;
	virtual void GetReordering(vector <int> & result) const;
	int GetNumIns() const { return nins_; }
	int GetNumDel() const { return ndel_; }
	virtual void PrintParse(const vector<string> & strs, ostream & out) const;
	void Print(ostream & out) const;
	virtual bool operator == (const DPState & other) const;
	void Rollback(const DPState * lstate);
protected:
	virtual DPState * Shift();
	// a delete action is also a reduce operation
	virtual DPState * Reduce(DPState * leftstate, Action action);
	// an idle action is also regarded as a insert operation
	virtual DPState * Insert(Action action);
	int nins_, ndel_;				// the number of insert/delete actions taken so far
};

}

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::IDPState & rhs )
{
    out << (rhs.IsGold() ? "*" : " ") << rhs.GetStep() << "("
    	<< (char)rhs.GetAction() << ", " << rhs.GetRank() << "):"
    	<< "< " << rhs.GetSrcL() << ", " << rhs.GetSrcR()-1 << ", "
		<< rhs.GetTrgL() << ", " << rhs.GetTrgR()-1 << " > :: "
		<< rhs.GetScore() << ", " << rhs.GetInside()
		<< ": left " << rhs.GetLeftPtrs().size()
		<< ", nins " << rhs.GetNumIns()
		<< ", ndel " << rhs.GetNumDel();
    return out;
}
}
#endif /* IDPSTATE_H_ */
