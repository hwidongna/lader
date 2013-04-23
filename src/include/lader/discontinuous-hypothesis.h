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
#include <lader/target-span.h>
using namespace std;

namespace lader {

class DiscontinuousHypothesis : public Hypothesis{
public:
//	DiscontinuousHypothesis(double viterbi_score, double single_score,
//	               int left, int m,
//	               int n, int right,
//	               int trg_left, int trg_right,
//	               HyperEdge::Type type, int center = -1,
//	               int left_rank = -1, int right_rank = -1,
//	               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
//	            	   Hypothesis(viterbi_score, single_score,
//	            			   new DiscontinuousHyperEdge(left, m, n, right, type),
//	            			   trg_left, trg_right,
//	            			   left_rank, right_rank,
//	            			   left_child, right_child) {}

	DiscontinuousHypothesis(const Hypothesis & hyp) : Hypothesis(hyp) {
//		Hypothesis(hyp.GetScore(), hyp.GetSingleScore(),
//		NULL, // initialized below
//		hyp.GetTrgLeft(), hyp.GetTrgRight(),
//		hyp.GetLeftRank(), hyp.GetRightRank(),
//		hyp.GetLeftChild(), hyp.GetRightChild()) {
////				Hypothesis(hyp) {
//		const DiscontinuousHyperEdge * dedge =
//				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
//		if (dedge == NULL)
//			THROW_ERROR("Invalide hyper-edge")
////		this->SetScore(hyp.GetScore());
////		this->SetSingleScore(hyp.GetSingleScore());
////		this->SetLoss(hyp.GetLoss());
//		this->SetEdge(new DiscontinuousHyperEdge(*dedge));
////		this->SetTrgLeft(hyp.GetTrgLeft());
////		this->SetTrgRight(hyp.GetTrgRight());
////		this->SetLeftRank(hyp.GetLeftRank());
////		this->SetRightRank(hyp.GetRightRank());
////		this->SetLeftChild(hyp.GetLeftChild());
////		this->SetRightChild(hyp.GetRightChild());
	}
	int GetM() {
		const DiscontinuousHyperEdge * dedge =
				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
		return dedge->GetM();
	}
	int GetN() {
		const DiscontinuousHyperEdge * dedge =
				dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
		if (dedge == NULL)
			THROW_ERROR("Invalide hyper-edge")
			return dedge->GetN();
	}


    // Comparators
    bool operator< (const Hypothesis & rhs) const {
    	const DiscontinuousHyperEdge * dedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(Hypothesis::GetEdge());
    	const DiscontinuousHyperEdge * rhsdedge =
    			dynamic_cast<const DiscontinuousHyperEdge*>(rhs.Hypothesis::GetEdge());
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
    			dynamic_cast<const DiscontinuousHyperEdge*>(rhs.Hypothesis::GetEdge());
    	if (dedge == NULL || rhsdedge == NULL)
    		THROW_ERROR("Invalide hyper-edge")
        return Hypothesis::operator== (rhs) &&
        		dedge->GetM() == rhsdedge->GetM() && dedge->GetN() == rhsdedge->GetN();
    }
};

}
#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
