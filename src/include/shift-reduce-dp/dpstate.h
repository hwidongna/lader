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
#include <lader/feature-vector.h>
using namespace std;

namespace lader {

#define MULTI_K 0xFFFF

class ReordererModel;
class FeatureSet;
class DPState;
typedef std::vector<DPState*> DPStateVector;

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
			ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	bool Allow(const Action & action, const int n);
	void AllActions(vector <Action> & result);
	void InsideActions(vector <Action> & result);
	DPState * Previous();
	DPState * GetLeftState() const;
	DPState * LeftChild() const;
	DPState * RightChild() const;
	double GetScore() const { return score_; }
	double GetInside() const { return inside_; }
	int GetI() const { return i_; }
	int GetJ() const { return j_; }
	int GetK() const { return k_; }
	int GetL() const { return l_; }
	int GetStep() const { return step_; }
	Action GetAction() const { return action_; }
	pair<int,int> GetSpan() { return MakePair(i_, j_-1); }
	pair<int,int> GetTargetSpan() { return MakePair(k_, l_-1); }
	void InsideReordering(vector<int> & result);
	void GetReordering(vector <int> & result);

	// a simple hash function
	size_t hash() const { return k_ * MULTI_K + l_; }
	bool operator == (const DPState & other) const {
		return k_ == other.k_ && l_ == other.l_ // they are signature
				&& i_ == other.i_; // do not need to compare j_
	}
	bool operator < (const DPState & other) const {
		return score_ < other.score_ || (score_ == other.score_ && inside_ < other.inside_);
	}
private:
	DPState * Shift();
	DPState * Reduce(DPState * leftstate, Action action);
	double score_, inside_, shiftcost_;
	int step_;
	int i_, j_; // source span
	int k_, l_; // target span
	int rank_;
	Action action_;
	bool gold_;
	vector<DPState*> leftptrs_;
	vector<BackPtr> backptrs_;
	bool keep_alternatives_;
	FeatureVectorInt * feat_;
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
    	<< "< " << rhs.GetI() << ", " << rhs.GetJ()-1 << ", "
		<< rhs.GetK() << ", " << rhs.GetL()-1 << " > :: "
		<< rhs.GetScore() << ", " << rhs.GetInside();
    return out;
}
}

#endif /* DPSTATE_H_ */
