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
	            			   left_rank, right_rank,
	            			   left_child, right_child) { }

	DiscontinuousHypothesis(double viterbi_score, double single_score, double non_local_score,
	               HyperEdge * edge,
	               int trg_left, int trg_right,
	               int left_rank = -1, int right_rank = -1,
	               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score, non_local_score,
	            			   edge,
	            			   trg_left, trg_right,
	            			   left_rank, right_rank,
	            			   left_child, right_child) { }

	DiscontinuousHypothesis(const Hypothesis & hyp) : Hypothesis(hyp) {	}
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


    // Comparators
    bool operator< (const Hypothesis & rhs) const {
//    	cerr << "operator<" << this->GetEdge() << endl;
    	DiscontinuousHyperEdge * dedge = // why does this cause NULL?
    			dynamic_cast<DiscontinuousHyperEdge*>(this->GetEdge());
    	DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<DiscontinuousHyperEdge*>(rhs.GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return Hypothesis::operator< (rhs) || (
        		Hypothesis::operator== (rhs) && (dedge->GetM() < rhsdedge->GetM() || (
				dedge->GetM() == rhsdedge->GetM() && dedge->GetN() < rhsdedge->GetN())));
    }
    bool operator== (const Hypothesis & rhs) const {
    	DiscontinuousHyperEdge * dedge =
    			dynamic_cast<DiscontinuousHyperEdge*>(this->GetEdge());
    	const DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<DiscontinuousHyperEdge*>(rhs.GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return Hypothesis::operator== (rhs) &&
        		dedge->GetM() == rhsdedge->GetM() && dedge->GetN() == rhsdedge->GetN();
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
		<< (char)rhs.GetEdgeType() << ", " << rhs.GetCenter() << " :: "
    	<< rhs.GetScore() << ", " << rhs.GetSingleScore() << ", " << rhs.GetNonLocalScore() << ">";
    return out;
}
}

#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
