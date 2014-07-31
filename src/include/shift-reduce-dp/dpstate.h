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
#include <typeinfo>
using namespace std;

namespace lader {

#define MULTI_K 0xFFFF

class DPStateNode;
class ReordererModel;
class FeatureSet;
class DPState;
typedef vector<DPState*> DPStateVector;
typedef pair<int,int> Span;
class DPState {
	friend class DDPState;
	friend class IDPState;
public:
	// compatible to the HyperEdge::Type
	typedef enum{
		INIT = 'R', // do not use except for the initial state
		SHIFT = 'F', 	// compatible to Forward of HyperEdge::Type
		STRAIGTH = 'S',
		INVERTED = 'I',
		DELETE_L = 'Z',	// for IDPState
		DELETE_R = 'X',	// for IDPState
		INSERT_L = 'C',	// for IDPState
		INSERT_R = 'V',	// for IDPState
		SWAP = 'D',		// for DDPState
		IDLE = 'E'		// for IDPState and DDPState
	} Action;

	static vector<Action> ActionFromString(const string & line);
	typedef struct {
		DPState * lchild;
		DPState * rchild;
		Action action;
		double cost;
	} BackPtr;

	DPState(); // for initial state
	virtual ~DPState();
	DPState(int step, int i, int j, Action action);

	bool IsGold() const { return gold_; }
	void SetRank(int rank) { rank_ = rank; }
	int GetRank() const { return rank_; }
	virtual void MergeWith(DPState * other);
	virtual void Take(Action action, DPStateVector & result, bool actiongold = false,
			int maxterm = 1, ReordererModel * model = NULL, const FeatureSet * feature_gen = NULL,
			const Sentence * sent = NULL);
	virtual bool Allow(const Action & action, const int n);
	virtual bool IsContinuous() { return true; }
	virtual void InsideActions(vector<Action> & result) const;
	void AllActions(vector <Action> & result) const;
	// Previous state
	virtual DPState * Previous() const;
	// Left state, s.t. can be reduced with this state
	DPState * GetLeftState() const;
	// Left child state
	virtual DPState * LeftChild() const;
	// Right child state
	virtual DPState * RightChild() const;
	double GetScore() const { return score_; }
	double GetInside() const { return inside_; }
	int GetSrcL() const { return src_l_; }
	int GetSrcR() const { return src_r_; }
	virtual int GetSrcREnd() const { return src_r_; }
	int GetSrcC() const { return src_c_; }
	int GetTrgL() const { return trg_l_; }
	int GetTrgR() const { return trg_r_; }
	int GetStep() const { return step_; }
	Action GetAction() const { return action_; }
	Span GetSrcSpan() const { return MakePair(src_l_, src_r_-1); }
	Span GetTrgSpan() const { return MakePair(trg_l_, trg_r_-1); }
	DPStateVector GetLeftPtrs() const { return leftptrs_; }
	virtual void GetReordering(vector <int> & result) const;
	void SetSignature(int max);
	vector<Span> GetSignature() const { return signature_; }
	void SetLoss(double loss) { loss_ = loss; }
	double GetLoss() const { return loss_; }
	void SetRescore(double rescore) { rescore_ = rescore; }
	double GetRescore() const { return rescore_; }

	// a simple hash function
	size_t hash() const { return trg_l_ * MULTI_K + trg_r_; }
	// compare signature
	bool operator != (const DPState & other) const {
		return !operator==(other);
	}
	virtual bool operator == (const DPState & other) const;
	bool operator < (const DPState & other) const {
		return score_ < other.score_ || (score_ == other.score_ && inside_ < other.inside_);
	}
	virtual void PrintParse(const vector<string> & strs, ostream & out) const;
	void PrintTrace(ostream & out) const;
	virtual void Print(ostream & out) const;
    std::string name() const { return typeid(*this).name(); }
	virtual DPStateNode * ToFlatTree();
protected:
	virtual DPState * Shift();
	virtual DPState * Reduce(DPState * leftstate, Action action);
	double score_, inside_, shiftcost_;
	int step_;
	int src_l_, src_r_;	// source span
	int src_c_; 		// the split point of subspans if exists, otherwise -1
	int trg_l_, trg_r_;	// target span
	vector<Span> signature_; // target spans in the stack (from the top)
	int rank_;			// the rank of this state in a bin
	Action action_;		// action taken
	bool gold_;			// whether this is a gold state or not
	vector<DPState*> leftptrs_; // a list of left states that can be reduced with this state
	vector<BackPtr> backptrs_;	// a list of previous states where this state comes from
	bool keep_alternatives_;
	double loss_;				// for loss-augmented training
	double rescore_;			// for loss-augmented training
};
typedef vector<DPState::Action> ActionVector;
typedef std::priority_queue<DPState*,
		DPStateVector, PointerLess<DPState*> > DPStateQueue;

struct DPStateHash {
std::size_t operator()( const DPState * c ) const
        {
            return c->hash();
        }
};

struct DPStateEqual {
std::size_t operator()( const DPState * s1, const DPState * s2 ) const
        {
            return *s1 == *s2;
        }
};

typedef std::tr1::unordered_map<DPState*, DPState*, DPStateHash, DPStateEqual> DPStateMap;

} /* namespace lader */

namespace std {
// Output function for pairs
inline ostream& operator << ( ostream& out,
                                   const lader::DPState & rhs )
{
    out << (rhs.IsGold() ? "*" : " ") << rhs.GetStep() << "("
    	<< (char)rhs.GetAction() << ", " << rhs.GetRank() << "):"
    	<< "< " << rhs.GetSrcL() << ", " << rhs.GetSrcR()-1 << ", "
		<< rhs.GetTrgL() << ", " << rhs.GetTrgR()-1 << " > :: "
		<< rhs.GetScore() << ", " << rhs.GetInside() << ": left " << rhs.GetLeftPtrs().size();
    return out;
}
}

#endif /* DPSTATE_H_ */
