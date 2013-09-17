/*
 * discontinuous-hypothesis.h
 *
 *  Created on: Apr 9, 2013
 *      Author: leona
 */

#ifndef DISCONTINUOUS_HYPOTHESIS_H_
#define DISCONTINUOUS_HYPOTHESIS_H_

#include <lader/hypothesis.h>
#include <lader/discontinuous-hyper-edge.h>
#include <lader/util.h>
#include <sstream>
#include <iostream>
using namespace std;

namespace lader {

class DiscontinuousHypothesis : public Hypothesis{

public:
    int NumSeq() {
    	if (seq_ > 0)
    		return seq_;
		DiscontinuousHypothesis *left =
				dynamic_cast<DiscontinuousHypothesis*>(GetLeftHyp());
		DiscontinuousHypothesis *right =
				dynamic_cast<DiscontinuousHypothesis*>(GetRightHyp());
		if (left && right)
			THROW_ERROR("Both children are discontinuous" << endl
					<< *left << endl << *right << endl);
		if (left)
			seq_ += left->NumSeq();
		else if (right)
			seq_ += right->NumSeq();
		return ++seq_;
	}

	DiscontinuousHypothesis(double viterbi_score, double single_score, double non_local_score,
			HyperEdge *edge, 			int trg_left, int trg_right, lm::ngram::State state,
			int left_rank = -1, int right_rank = -1,
			TargetSpan *left_child = NULL, TargetSpan *right_child = NULL) :
				Hypothesis(viterbi_score, single_score, non_local_score,
					edge, trg_left, trg_right, state,
					left_rank, right_rank,
					left_child, right_child), seq_(0) { }
protected:
	friend class TestDiscontinuousHyperGraph;
	// just for testing
	DiscontinuousHypothesis(double viterbi_score, double single_score, double non_local_score,
					int left, int m,
					int n, int right,
					int trg_left, int trg_right,
					HyperEdge::Type type, int center = -1,
					int left_rank = -1, int right_rank = -1,
					TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score, non_local_score,
	            			   new DiscontinuousHyperEdge(left, m, center, n, right, type),
	            			   trg_left, trg_right,
	            			   lm::ngram::State(),
	            			   left_rank, right_rank,
	            			   left_child, right_child), seq_(0) { }
	// just for testing
	DiscontinuousHypothesis(double viterbi_score, double single_score, double non_local_score,
	               HyperEdge * edge,
	               int trg_left, int trg_right,
	               int left_rank = -1, int right_rank = -1,
	               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score, non_local_score,
	            			   edge,
	            			   trg_left, trg_right,
	            			   lm::ngram::State(),
	            			   left_rank, right_rank,
	            			   left_child, right_child), seq_(0) { }
public:
//	DiscontinuousHypothesis(const Hypothesis & hyp) : Hypothesis(hyp) {	}
	int GetM() const{
		DiscontinuousHyperEdge * dedge =
				dynamic_cast<DiscontinuousHyperEdge*>(this->GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
		return dedge->GetM();
	}
	int GetN() const{
		DiscontinuousHyperEdge * dedge =
				dynamic_cast<DiscontinuousHyperEdge*>(this->GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
		return dedge->GetN();
	}

    virtual Hypothesis *Clone() const{
        DiscontinuousHyperEdge *dedge =
        		dynamic_cast<DiscontinuousHyperEdge*>(this->GetEdge());
        if (dedge == NULL)
        	THROW_ERROR("Invalide hyper-edge")
    	Hypothesis * new_hyp = new DiscontinuousHypothesis(*this);
    	new_hyp->SetEdge(dedge->Clone());
    	return new_hyp;
    }

    // We can skip this hypothesis in search
    // if this hypothesis produce an ITG permutation from discontinuous children
    static bool CanSkip(const HyperEdge * edge,
    		const Hypothesis * left, const Hypothesis * right) {
    	const DiscontinuousHypothesis * dleft =
    			dynamic_cast<const DiscontinuousHypothesis*>(left);
    	const DiscontinuousHypothesis * dright =
    			dynamic_cast<const DiscontinuousHypothesis*>(right);
    	// continuous + discotinuous = discontinuous
    	// discontinuous + cotinuous = discontinuous
    	return (dleft && edge->GetType() == dleft->GetEdgeType())
    			|| (dright && edge->GetType() == dright->GetEdgeType());
    }
private:
	// the number of sequenctial discontinuous derivations
	int seq_;
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator << ( std::ostream& out,
                                   const lader::DiscontinuousHypothesis & rhs )
{
    out << "<" << rhs.GetLeft() << ", " << rhs.GetM() << ", "
		<< rhs.GetN() << ", " << rhs.GetRight() << ", "
		<< rhs.GetTrgLeft() << ", " << rhs.GetTrgRight() << ", "
		<< (char) rhs.GetEdgeType() << (char) rhs.GetEdge()->GetClass() << ", "
		<< rhs.GetCenter() << " :: "
    	<< rhs.GetScore() << ", " << rhs.GetSingleScore() << ", " << rhs.GetNonLocalScore() << ">";
	vector<int> reorder;
	rhs.GetReordering(reorder);
	BOOST_FOREACH(int i, reorder)
		out << " " << i;
    return out;
}
}

#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
