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
	DiscontinuousHypothesis(double viterbi_score, double single_score,
			HyperEdge *edge, int trg_left, int trg_right,
			int left_rank = -1, int right_rank = -1,
			TargetSpan *left_child = NULL, TargetSpan *right_child = NULL) :
				Hypothesis(viterbi_score, single_score,
					edge, trg_left, trg_right,
					left_rank, right_rank,
					left_child, right_child) { }
protected:
	friend class TestDiscontinuousHyperGraph;
	// just for testing
	DiscontinuousHypothesis(double viterbi_score, double single_score,
					int left, int m,
					int n, int right,
					int trg_left, int trg_right,
					HyperEdge::Type type, int center = -1,
					int left_rank = -1, int right_rank = -1,
					TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score,
	            			   new DiscontinuousHyperEdge(left, m, center, n, right, type),
	            			   trg_left, trg_right,
	            			   left_rank, right_rank,
	            			   left_child, right_child) { }
public:

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
    bool operator== (const Hypothesis & rhs) const {
    	const DiscontinuousHyperEdge * dedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
    	const DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(rhs.GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return dedge->GetM() == rhsdedge->GetM() && dedge->GetN() == rhsdedge->GetN()
        		&& Hypothesis::operator== (rhs);
    }
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
		<< (char)rhs.GetEdgeType() << (char) rhs.GetEdge()->GetClass() << ", "
		<< rhs.GetCenter() << " :: "
    	<< rhs.GetScore() << ", " << rhs.GetSingleScore() << ">";
	vector<int> reorder;
	rhs.GetReordering(reorder);
	BOOST_FOREACH(int i, reorder)
		out << " " << i;
    return out;
}
}

#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
