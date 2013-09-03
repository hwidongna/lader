#ifndef TARGET_SPAN_H__ 
#define TARGET_SPAN_H__

#include <boost/foreach.hpp>
#include <lader/hypothesis.h>
#include <lader/symbol-set.h>
#include <lader/util.h>
#include <vector>
#include <cfloat>
#include <set>
#include <sstream>
#include <iostream>

using namespace std;
namespace lader {

class TargetSpan {
public:
    TargetSpan(int left, int right, int trg_left=-1, int trg_right=-1)
                    : left_(left), right_(right),
                      id_(-1) { }
    virtual ~TargetSpan() {
        BOOST_FOREACH(Hypothesis * hyp, hyps_)
            delete hyp;
        while(cands_.size()) {
        	delete cands_.top();
        	cands_.pop();
        }
    }

    void LabelWithIds(int & curr_id){
    	if(id_ != -1) return;
    	BOOST_FOREACH(Hypothesis * hyp, hyps_) {
    		if(hyp->GetLeftChild())
    			hyp->GetLeftChild()->LabelWithIds(curr_id);
    		if(hyp->GetRightChild())
    			hyp->GetRightChild()->LabelWithIds(curr_id);
    	}
    	id_ = curr_id++;
    }

    Hypothesis * LazyKthBest(int k,
    		ReordererModel & model,
    		const FeatureSet & features, const FeatureSet & non_local_features,
    		const Sentence & sent){
    	while ((int)hyps_.size() < k+1 && (int)cands_.size() > 0){
    		Hypothesis * hyp = cands_.top(); cands_.pop();
    		hyps_.push_back(hyp);
    		hyp->LazyNext(cands_, model, features, non_local_features, sent);
            // If the next hypothesis on the stack is equal to the current
            // hypothesis, remove it, as this just means that we added the same
            // hypothesis
            while(cands_.size() && *cands_.top() == *hyp) {
            	delete cands_.top();
            	cands_.pop();
            }
    	}
    	if ( k < (int)hyps_.size()){
    		return hyps_[k];
    	}
    	return NULL;
    }

    double GetScore() const {
        double ret = !hyps_.size() ? -DBL_MAX : hyps_[0]->GetScore();
        return ret;
    }
    int GetId() const { return id_; }
    int GetLeft() const { return left_; }
    int GetRight() const { return right_; }
    // Add a hypothesis with a new hyper-edge
    void AddHypothesis(Hypothesis * hyp) { hyps_.push_back(hyp); }
    const std::set<char> & GetHasTypes() { return has_types_; }
    const std::vector<Hypothesis*> & GetHypotheses() const { return hyps_; }
    std::vector<Hypothesis*> & GetHypotheses() { return hyps_; }
    const Hypothesis* GetHypothesis(int i) const { return SafeAccess(hyps_,i); }
    Hypothesis* GetHypothesis(int i) {
        if((int)hyps_.size() <= i)
            return NULL;
        else
        	return hyps_[i];
    }
    HypothesisQueue & GetCands() { return cands_; }
    size_t size() const { return hyps_.size(); }
    const Hypothesis* operator[] (size_t val) const { return hyps_[val]; }
    Hypothesis* operator[] (size_t val) { return hyps_[val]; }
    void ResetId() { id_ = -1; has_types_.clear(); }
    void SetId(int id) { id_ = id; }
    void SetHasType(char c) { has_types_.insert(c); }
    
protected:
    std::vector<Hypothesis*> hyps_;
    HypothesisQueue cands_;

private:
    int left_, right_;
    int id_;
    std::set<char> has_types_;
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator << ( std::ostream& out,
                                   const lader::TargetSpan & rhs )
{
    out << "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ">";
    return out;
}
}

#endif

