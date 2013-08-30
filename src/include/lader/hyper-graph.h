#ifndef HYPER_GRAPH_H__ 
#define HYPER_GRAPH_H__

#include <iostream>
#include <lader/target-span.h>
#include <lader/feature-vector.h>
#include <lader/feature-data-base.h>
#include <lader/hyper-edge.h>
#include <lader/util.h>
#include <lader/loss-base.h>
#include <tr1/unordered_map>

namespace lader {

class ReordererModel;
class FeatureSet;


template <class T>
struct DescendingScore {
  bool operator ()(T *lhs, T *rhs) { return rhs->GetScore() < lhs->GetScore(); }
};

class HyperGraph {
public:
    
    friend class TestHyperGraph;
    friend class TestDiscontinuousHyperGraph;

    HyperGraph() : features_(0), n_(-1) { }

    virtual ~HyperGraph() {
        if(features_) {
            BOOST_FOREACH(EdgeFeaturePair efp, *features_){
            	delete efp.first;
                delete efp.second;
            }
            delete features_;
        }
        BOOST_FOREACH(TargetSpan * stack, stacks_)
            delete stack;
    }
    
    // Build the hypergraph using the specified model, features and sentence
    //  beam_size: the pop limit for cube pruning
    void BuildHyperGraph(ReordererModel & model,
                         const FeatureSet & features,
                         const FeatureSet & non_local_features,
                         const Sentence & sent,
                         int beam_size = 0);

    const TargetSpan * GetStack(int l, int r) const {
        return SafeAccess(stacks_, GetTrgSpanID(l,r));
    }
    TargetSpan * GetStack(int l, int r) {
        return SafeAccess(stacks_, GetTrgSpanID(l,r));
    }
    const std::vector<TargetSpan*> & GetStacks() const { return stacks_; }
    std::vector<TargetSpan*> & GetStacks() { return stacks_; }


    // Get a loss-augmented score for a hypothesis
    // Do not change the original score
    double Score(double loss_multiplier, const Hypothesis* hyp) const;

    // Print the whole hypergraph in JSON format
    void PrintHyperGraph(const std::vector<std::string> & sent,
                         std::ostream & out);

    // Rescore the hypergraph using the given model and a loss multiplier
    // Keep the hypergraph structure stored in the stacks except the root
    double Rescore(double loss_multiplier);

    const TargetSpan * GetRoot() const {
        return SafeAccess(stacks_, stacks_.size()-1);
    }
    TargetSpan * GetRoot() {
        return SafeAccess(stacks_, stacks_.size()-1);
    }
    Hypothesis * GetBest() {
    	return SafeAccess(stacks_, stacks_.size()-1)->GetHypothesis(0);
    }
    // Add up the loss over an entire sentence
    virtual void AddLoss(
    		LossBase* loss,
    		const Ranks * ranks,
            const FeatureDataParse * parse) const;

    void SetFeatures(EdgeFeatureMap * features) { features_ = features;}
    EdgeFeatureMap * GetFeatures() { return features_; }
    // Clear the feature array without deleting the features themselves
    // This is useful if you want to save the features for later use
    void ClearFeatures() { features_ = NULL; }

    void AccumulateNonLocalFeatures(std::tr1::unordered_map<int,double> & feat_map,
						ReordererModel & model,
						const FeatureSet & feature_gen,
						const Sentence & sent,
						const Hypothesis & hyp);
protected:
    virtual TargetSpan * ProcessOneSpan(ReordererModel & model,
                               const FeatureSet & features,
                               const FeatureSet & non_local_features,
                               const Sentence & sent,
                               int l, int r,
                               int beam_size = 0);


    const FeatureVectorInt * GetEdgeFeatures(
                                ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge);

    // only for test
    void SetEdgeFeatures(const HyperEdge & edge, FeatureVectorInt * feat) {
        if(!features_) features_ = new EdgeFeatureMap;
        features_->insert(MakePair(edge.Clone(), feat));
    }

    double GetEdgeScore(ReordererModel & model,
                        const FeatureSet & feature_gen,
                        const Sentence & sent,
                        const HyperEdge & edge);

    double GetNonLocalScore(ReordererModel & model,
						const FeatureSet & feature_gen,
						const Sentence & sent,
						const Hypothesis & left,
						const Hypothesis & right);

    inline int GetTrgSpanID(int l, int r) const { 
        // If l=-1, we want the root, so return the last element of stacks_
        if(l == -1) return stacks_.size() - 1;
        return r*(r+1)/2 + l;
    }

    virtual void SetStack(int l, int r, TargetSpan * stack) {
#ifdef LADER_SAFE
        if(l < 0 || r < 0)
            THROW_ERROR("Bad SetStack (l="<<l<<", r="<<r<<")"<<std::endl);
#endif
        int idx = HyperGraph::GetTrgSpanID(l,r);
        if((int)stacks_.size() <= idx)
            stacks_.resize(idx+1, NULL);
        if(stacks_[idx]) delete stacks_[idx];
        stacks_[idx] = stack;
    }

private:

    // A map containing feature vectors for each of the edges
    EdgeFeatureMap * features_;
    // Stacks containing the hypotheses for each span
    // The indexing for the outer vector is:
    //  0-0 -> 0, 0-1 -> 1, 1-1 -> 2, 0-2 -> 3 ...
    // And can be recovered by GetHypothesis.
    // The inner vector contains target spans in descending rank of score
    std::vector<TargetSpan*> stacks_;

protected:
    // The length of the sentence
    int n_;

};

}

#endif

