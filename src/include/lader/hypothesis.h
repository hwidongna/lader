#ifndef HYPOTHESIS_H__ 
#define HYPOTHESIS_H__

#include <lader/hyper-edge.h>
#include <lader/feature-vector.h>
#include <lader/util.h>
#include <sstream>
#include <iostream>

namespace lader {

typedef std::tr1::unordered_map<const HyperEdge*, FeatureVectorInt*,
		PointerHash<const HyperEdge*>, PointerEqual<const HyperEdge*> > EdgeFeatureMap;
typedef std::pair<const HyperEdge*, FeatureVectorInt*> EdgeFeaturePair;

class TargetSpan;

// A tuple that is used to hold hypotheses during cube pruning
class Hypothesis {
public:
    Hypothesis(double viterbi_score, double single_score, double non_local_score,
               HyperEdge * edge,
               int trg_left, int trg_right,
               int left_rank = -1, int right_rank = -1,
               TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
               viterbi_score_(viterbi_score),
               single_score_(single_score),
               non_local_score_(non_local_score),
               loss_(0),
               edge_(edge),
               trg_left_(trg_left), trg_right_(trg_right),
               left_child_(left_child), right_child_(right_child),
               left_rank_(left_rank), right_rank_(right_rank)
               { } //std::cerr << "Hypothesis " << GetEdge() << std::endl; }

    Hypothesis(double viterbi_score, double single_score, double non_local_score,
				int left, int right,
				int trg_left, int trg_right,
				HyperEdge::Type type, int center = -1,
				int left_rank = -1, int right_rank = -1,
				TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
				viterbi_score_(viterbi_score),
				single_score_(single_score),
				non_local_score_(non_local_score),
				loss_(0),
				edge_(new HyperEdge(left, center, right, type)),
				trg_left_(trg_left), trg_right_(trg_right),
				left_child_(left_child), right_child_(right_child),
				left_rank_(left_rank), right_rank_(right_rank)
				{ }

    // be aware of the pointer member edge_
    // After calling this method, you need to allocate a dynamic memory for edge_
    // It is implicitly called (may be several times) when inserting an instance into std::containers
    // For that case, we do not allocate a dynamic memory in this method, which lead a dangling pointer
    Hypothesis(const Hypothesis & hyp) :
				viterbi_score_(hyp.viterbi_score_),
				single_score_(hyp.single_score_),
				non_local_score_(hyp.non_local_score_),
				loss_(hyp.loss_),
				edge_(hyp.edge_),
				trg_left_(hyp.trg_left_), trg_right_(hyp.trg_right_),
				left_child_(hyp.left_child_), right_child_(hyp.right_child_),
				left_rank_(hyp.left_rank_), right_rank_(hyp.right_rank_)
    			{ }

    virtual ~Hypothesis() {
    	if (edge_ != NULL){
    		delete edge_;
    	}
//    	if (left_child_ != NULL)
//    		delete left_child_;
//    	if (right_child_ != NULL)
//    		delete right_child_;
    }
    // Get a string representing the rule of this hypothesis
    std::string GetRuleString(const std::vector<std::string> & sent,
                              char left_val = 0, char right_val = 0) const {
        std::ostringstream ret;
        ret << "[" << (char)GetEdgeType() << "] |||";
        if(left_val) {
            ret << " ["<<left_val<<"]";
            if(right_val) ret << " ["<<right_val<<"]";
        } else {
            for(int i = GetLeft(); i <= GetRight(); i++) {
                ret << " ";
                for(int j = 0; j < (int)sent[i].length(); j++) {
                    if(sent[i][j] == '\\' || sent[i][j] == '\"') ret << "\\";
                    ret << sent[i][j];
                }
            }
        }
        return ret.str();
    }

    // Comparators
    virtual bool operator< (const Hypothesis & rhs) const {
        return 
            viterbi_score_ < rhs.viterbi_score_ || 
            (viterbi_score_ == rhs.viterbi_score_ && (
            trg_left_ < rhs.trg_left_ || (trg_left_ == rhs.trg_left_ && (
            trg_right_ < rhs.trg_right_ || (trg_right_ == rhs.trg_right_ && (
            GetEdgeType() < rhs.GetEdgeType() || (GetEdgeType() == rhs.GetEdgeType() && (
            GetCenter() < rhs.GetCenter() || (GetCenter() == rhs.GetCenter() && (
            left_child_ < rhs.left_child_ || (left_child_ == rhs.left_child_ && (
            right_child_ < rhs.right_child_))))))))))));
    }
    virtual bool operator== (const Hypothesis & rhs) const {
        return
            viterbi_score_ == rhs.viterbi_score_ &&
            trg_left_ == rhs.trg_left_ &&
            trg_right_ == rhs.trg_right_ &&
            GetEdgeType() == rhs.GetEdgeType() &&
            GetCenter() == rhs.GetCenter() &&
            left_child_ == rhs.left_child_ &&
            right_child_ == rhs.right_child_;
    }

    // Accessors
    double GetScore() const { return viterbi_score_; }
    double GetSingleScore() const { return single_score_; }
    double GetNonLocalScore() const { return non_local_score_; }
    double GetLoss() const { return loss_; }
    HyperEdge * GetEdge() const { return edge_; }
    int GetLeft() const { return edge_->GetLeft(); }
    int GetRight() const { return edge_->GetRight(); }
    int GetTrgLeft() const { return trg_left_; }
    int GetTrgRight() const { return trg_right_; }
    int GetCenter() const { return edge_->GetCenter(); }
    HyperEdge::Type GetEdgeType() const { return edge_->GetType(); }
    TargetSpan* GetLeftChild() const { return left_child_; }
    TargetSpan* GetRightChild() const { return right_child_; }
    int GetLeftRank() const { return left_rank_; }
    int GetRightRank() const { return right_rank_; }
    Hypothesis * GetLeftHyp() const;
    Hypothesis * GetRightHyp() const;

    void SetScore(double dub) { viterbi_score_ = dub; }
    void SetSingleScore(double dub) { single_score_ = dub; }
    void SetNonLocalScore(double dub) { non_local_score_ = dub; }
    void SetLoss(double dub) { loss_ = dub; }
    void SetEdge(HyperEdge * edge) { edge_ = edge; }
    void SetLeftChild (TargetSpan* dub)  { left_child_ = dub; }
    void SetRightChild(TargetSpan* dub) { right_child_ = dub; }
    void SetLeftRank (int dub)  { left_rank_ = dub; }
    void SetRightRank(int dub) { right_rank_ = dub; }
    void SetTrgLeft(int dub) { trg_left_ = dub; }
    void SetTrgRight(int dub) { trg_right_ = dub; }
    void SetType(HyperEdge::Type type) { edge_->SetType(type); } // only for testing
    virtual void PrintChildren( std::ostream& out ) const;

    // Add up the loss over an entire subtree defined by this hyp
    double AccumulateLoss();
    FeatureVectorInt AccumulateFeatures(
        		const EdgeFeatureMap * features);
    FeatureVectorInt AccumulateFeatures(
    		std::tr1::unordered_map<int,double> & feat_map,
    		const EdgeFeatureMap * features);

private:
    void AccumulateFeatures(const EdgeFeatureMap * features,
                            std::tr1::unordered_map<int,double> & feat_map);
    double viterbi_score_; // The Viterbi score for the entire subtree that
                           // this hypothesis represents
    double single_score_;  // The score for only this edge
    double non_local_score_;  // The score for only this hypothesis
    double loss_;          // The loss for the single action represented
                           // by this hypothesis
    HyperEdge * edge_ ;	// The corresponding hyper-edge of the production
    int trg_left_, trg_right_; // The target words that fall on the far left
                               // and far right of this span
    TargetSpan *left_child_, *right_child_; // The child hypothese for nonterm
    int left_rank_, right_rank_; // The ranks of the left and right hypotheses
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator << ( std::ostream& out, 
                                   const lader::Hypothesis & rhs )
{
    out << "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ", "
    	<< rhs.GetTrgLeft() << ", " << rhs.GetTrgRight() << ", "
    	<< (char)rhs.GetEdgeType() << ", " << rhs.GetCenter() << " :: "
    	<< rhs.GetScore() << ", " << rhs.GetSingleScore() << ", " << rhs.GetNonLocalScore() << ">";
    return out;
}
}


#endif

