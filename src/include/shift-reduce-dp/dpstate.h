/*
 * dpstate.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef DPSTATE_H_
#define DPSTATE_H_

#include <vector>
#include <lader/hypothesis-queue.h>
#include <lader/feature-data-base.h>
#include <lader/util.h>
#include <queue>
#include <tr1/unordered_map>
using namespace std;

namespace lader {

#define MULTI_K 0xFFFF

class ReordererModel;
class FeatureSet;
class DPState;
typedef vector<DPState*> DPStateVector;
typedef pair<int,int> Span;
class DPState {
public:
	typedef enum{
		INIT = -1, // do not use except for the initial state
		SHIFT,
		STRAIGTH,
		INVERTED,
		NOP, // for iteration end point
	} Action;

	typedef struct {
		DPState * leftstate;
		DPState * istate;
		Action action;
		double cost;
	} BackPtr;

	DPState(); // for initial state
	virtual ~DPState();
	DPState(int step, int i, int j, Action action);

	bool IsGold() const { return gold_; }
	void SetRank(int rank) { rank_ = rank; }
	int GetRank() const { return rank_; }
	void MergeWith(DPState * other);
	void Take(Action action, DPStateVector & result, bool actiongold = false,
			int maxterm = 1, ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	bool Allow(const Action & action, const int n);
	void InsideActions(vector<Action> & result);
	void AllActions(vector <Action> & result);
	DPState * Previous();
	DPState * GetLeftState() const;
	DPState * LeftChild() const;
	DPState * RightChild() const;
	double GetScore() const { return score_; }
	double GetInside() const { return inside_; }
	int GetSrcL() const { return src_l_; }
	int GetSrcR() const { return src_r_; }
	int GetTrgL() const { return trg_l_; }
	int GetTrgR() const { return trg_r_; }
	int GetStep() const { return step_; }
	Action GetAction() const { return action_; }
	Span GetSrcSpan() const { return MakePair(src_l_, src_r_-1); }
	Span GetTrgSpan() const { return MakePair(trg_l_, trg_r_-1); }
	vector<DPState*> GetLeftPtrs() const { return leftptrs_; }
	void GetReordering(vector <int> & result);
	void SetSignature(int max);
	vector<Span> GetSignature() const { return signature_; }

	// a simple hash function
	size_t hash() const { return trg_l_ * MULTI_K + trg_r_; }
	// compare signature
	bool operator == (const DPState & other) const {
		if (signature_.size() != other.signature_.size())
			return false;
		return std::equal(signature_.begin(), signature_.end(), other.signature_.begin())
				&& src_l_ == other.src_l_; // do not need to compare j_
	}
	bool operator < (const DPState & other) const {
		return score_ < other.score_ || (score_ == other.score_ && inside_ < other.inside_);
	}
	void PrintParse(const vector<string> & strs, ostream & out) const;
private:
	DPState * Shift();
	DPState * Reduce(DPState * leftstate, Action action);
	double score_, inside_, shiftcost_;
	int step_;
	int src_l_, src_r_; // source span
	int trg_l_, trg_r_; // target span
	vector<Span> signature_; // target spans in the stack (from the top)
	int rank_;
	Action action_;
	bool gold_;
	vector<DPState*> leftptrs_;
	vector<BackPtr> backptrs_;
	bool keep_alternatives_;
};

typedef std::priority_queue<DPState*,
		DPStateVector, PointerLess<DPState*> > DPStateQueue;

struct DPStateHash {
std::size_t operator()( const DPState & c ) const
        {
            return c.hash();
        }
};

typedef std::tr1::unordered_map<DPState, DPState*, DPStateHash> DPStateMap;

} /* namespace lader */

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::DPState & rhs )
{
    out << (rhs.IsGold() ? "*" : " ") << rhs.GetStep() << "("
    	<< rhs.GetAction() << ", " << rhs.GetRank() << "):"
    	<< "< " << rhs.GetSrcL() << ", " << rhs.GetSrcR()-1 << ", "
		<< rhs.GetTrgL() << ", " << rhs.GetTrgR()-1 << " > :: "
		<< rhs.GetScore() << ", " << rhs.GetInside() << ": left " << rhs.GetLeftPtrs().size();
    return out;
}
}

#endif /* DPSTATE_H_ */