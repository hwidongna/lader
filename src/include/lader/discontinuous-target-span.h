#ifndef DISCONTINUOUS_TARGET_SPAN_H__
#define DISCONTINUOUS_TARGET_SPAN_H__

#include <lader/target-span.h>
#include <lader/discontinuous-hypothesis.h>

namespace lader {

class DiscontinuousTargetSpan : public TargetSpan{
public:
    DiscontinuousTargetSpan(int left, int m, int n, int right, int trg_left, int trg_right)
                    : TargetSpan(left, right, trg_left, trg_right), m_(m), n_(n) { }

    void AddHypothesis(const Hypothesis & hyp) {
    	DiscontinuousHypothesis * new_hyp = new DiscontinuousHypothesis(hyp);
    	new_hyp->SetEdge(new DiscontinuousHyperEdge(
    			new_hyp->GetLeft(), new_hyp->GetM(),
    			new_hyp->GetN(), new_hyp->GetRight(),
    			new_hyp->GetEdgeType()));
    	hyps_.push_back(new_hyp);
    }
    
    int GetM() { return m_; }
    int GetN() { return n_; }
private:

    int m_, n_;
};

}

#endif

