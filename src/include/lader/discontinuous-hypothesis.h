/*
 * discontinuous-hypothesis.h
 *
 *  Created on: Apr 9, 2013
 *      Author: leona
 */

#ifndef DISCONTINUOUS_HYPOTHESIS_H_
#define DISCONTINUOUS_HYPOTHESIS_H_

#include <lader/hypothesis.h>

class DiscontinuousHypothesis : public Hypothesis{
public:
	DiscontinuousHypothesis(double viterbi_score, double single_score,
	               int left, int m,
	               int n, int right,
	               int trg_left, int trg_right,
	               HyperEdge::Type type, int center = -1,
	               int left_rank = -1, int right_rank = -1,
	               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
	            	   Hypothesis(viterbi_score, single_score,
	            			   left, right,
	            			   trg_left, trg_right,
	            			   type, center,
	            			   left_rank, right_rank,
	            			   left_child, right_child), m_(m), n_(n) {}
	DiscontinuousHypothesis(const Hypothesis & hyp) : Hypothesis(hyp) {
		m_ = ((DiscontinuousHypothesis)hyp).m_;
		n_ = ((DiscontinuousHypothesis)hyp).n_;
	}
	int GetM() { return m_; }
	int GetN() { return n_; }
private:
	int m_, n_;
};
#endif /* DISCONTINUOUS_HYPOTHESIS_H_ */
