#ifndef DISCONTINUOUS_TARGET_SPAN_H__
#define DISCONTINUOUS_TARGET_SPAN_H__

#include <lader/target-span.h>

namespace lader {

class DiscontinuousTargetSpan : public TargetSpan{
public:
    DiscontinuousTargetSpan(int left, int m, int n, int right, int trg_left, int trg_right)
                    : TargetSpan(left, right, trg_left, trg_right), m_(m), n_(n) { }

    void AddHypothesis(const Hypothesis & hyp) {
    	hyps_.push_back(new DiscontinuousHypothesis(hyp));
    }
//    void GetReordering(std::vector<int> & reord) const { // TODO: override
//    	TargetSpan::GetReordering(reord);
//        HyperEdge::Type type = hyps_[0]->GetType();
//        if(type == HyperEdge::EDGE_FOR) {
//            for(int i = left_; i <= right_; i++)
//                reord.push_back(i);
//        } else if(type == HyperEdge::EDGE_BAC) {
//            for(int i = right_; i >= left_; i--)
//                reord.push_back(i);
//        } else if(type == HyperEdge::EDGE_ROOT) {
//            hyps_[0]->GetLeftChild()->GetReordering(reord);
//        } else if(type == HyperEdge::EDGE_STR) {
//            hyps_[0]->GetLeftChild()->GetReordering(reord);
//            hyps_[0]->GetRightChild()->GetReordering(reord);
//        } else if(type == HyperEdge::EDGE_INV) {
//            hyps_[0]->GetRightChild()->GetReordering(reord);
//            hyps_[0]->GetLeftChild()->GetReordering(reord);
//        }
//    }
    
    int GetM() { return m_; }
    int GetN() { return n_; }
private:

    int m_, n_;
};

}

#endif

