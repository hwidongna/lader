#ifndef TARGET_SPAN_H__ 
#define TARGET_SPAN_H__

#include <boost/foreach.hpp>
#include <lader/hypothesis.h>
#include <lader/discontinuous-hypothesis.h>
#include <lader/symbol-set.h>
#include <lader/util.h>
#include <vector>
#include <cfloat>
#include <set>
#include <sstream>
#include <iostream>

using namespace std;
namespace lader {

class HyperGraph;

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

    int GetId() const { return id_; }
    int GetLeft() const { return left_; }
    int GetRight() const { return right_; }
    // Add a hypothesis with a new hyper-edge
    void AddHypothesis(Hypothesis * hyp) { hyps_.push_back(hyp); }
    // Add an unique hypothesis with a new hyper-edge
    bool AddUniqueHypothesis(Hypothesis * hyp) {
    	BOOST_FOREACH(Hypothesis * prev, hyps_)
			if (*prev == *hyp)
				return false;
    	hyps_.push_back(hyp);
    	return true;
    }
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
    size_t CandSize() const { return cands_.size(); }
    const Hypothesis* operator[] (size_t val) const { return hyps_[val]; }
    Hypothesis* operator[] (size_t val) { return hyps_[val]; }
    void ResetId() { id_ = -1; has_types_.clear(); }
    void SetId(int id) { id_ = id; }
    void SetHasType(char c) { has_types_.insert(c); }
    
protected:
    std::vector<Hypothesis*> hyps_;
    // For cube growing
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

