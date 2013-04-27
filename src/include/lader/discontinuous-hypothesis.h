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
	               HyperEdge * edge,
	               int trg_left, int trg_right,
	               int left_rank = -1, int right_rank = -1,
	               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score,
	            			   edge,
	            			   trg_left, trg_right,
	            			   left_rank, right_rank,
	            			   left_child, right_child) {
//		cerr << "DiscontinuousHypothesis " << Hypothesis::GetEdge() << endl;
//		const DiscontinuousHyperEdge * dedge = // this doesn't cause NULL
//				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
//		if (dedge == NULL)
//			THROW_ERROR("Invalide hyper-edge")
//		else
//			cerr << *this << endl;
	}

	DiscontinuousHypothesis(const Hypothesis & hyp) : Hypothesis(hyp) {
//		Hypothesis(hyp.GetScore(), hyp.GetSingleScore(),
//		NULL, // initialized below
//		hyp.GetTrgLeft(), hyp.GetTrgRight(),
//		hyp.GetLeftRank(), hyp.GetRightRank(),
//		hyp.GetLeftChild(), hyp.GetRightChild()) {
//		cerr << "DiscontinuousHypothesis(const Hypothesis & hyp) " << Hypothesis::GetEdge() << endl;
//		const DiscontinuousHyperEdge * dedge =
//				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
//		if (dedge == NULL)
//			THROW_ERROR("Invalide hyper-edge")
////		this->SetScore(hyp.GetScore());
////		this->SetSingleScore(hyp.GetSingleScore());
////		this->SetLoss(hyp.GetLoss());
//		cerr << "copy a new edge" << dedge << endl;
//		this->SetEdge(new DiscontinuousHyperEdge(*dedge));
////		this->SetTrgLeft(hyp.GetTrgLeft());
////		this->SetTrgRight(hyp.GetTrgRight());
////		this->SetLeftRank(hyp.GetLeftRank());
////		this->SetRightRank(hyp.GetRightRank());
////		this->SetLeftChild(hyp.GetLeftChild());
////		this->SetRightChild(hyp.GetRightChild());
	}
	int GetM() const{
		const DiscontinuousHyperEdge * dedge =
				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
		return dedge->GetM();
	}
	int GetN() const{
		const DiscontinuousHyperEdge * dedge =
				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
			return dedge->GetN();
	}


    // Comparators
    bool operator< (const Hypothesis & rhs) const {
//    	cerr << "operator<" << Hypothesis::GetEdge() << endl;
    	const DiscontinuousHyperEdge * dedge = // why does this cause NULL?
    			dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
    	const DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(rhs.GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return Hypothesis::operator< (rhs) || (
        		Hypothesis::operator== (rhs) && (dedge->GetM() < rhsdedge->GetM() || (
				dedge->GetM() == rhsdedge->GetM() && dedge->GetN() < rhsdedge->GetN())));
    }
    bool operator== (const Hypothesis & rhs) const {
    	const DiscontinuousHyperEdge * dedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
    	const DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(rhs.GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return Hypothesis::operator== (rhs) &&
        		dedge->GetM() == rhsdedge->GetM() && dedge->GetN() == rhsdedge->GetN();
    }
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator << ( std::ostream& out,
                                   const lader::DiscontinuousHypothesis & rhs )
{
    out << "<" << rhs.GetLeft() << ", " << rhs.GetM() << ", "  << rhs.GetN() << ", " << rhs.GetRight()
    	<< ", " << rhs.GetTrgLeft() << ", " << rhs.GetTrgRight() << ", " << (char)rhs.GetEdgeType() << ", " << rhs.GetCenter() << ">";
    return out;
}
}

#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
