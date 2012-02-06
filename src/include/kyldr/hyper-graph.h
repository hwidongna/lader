#ifndef HYPER_GRAPH_H__ 
#define HYPER_GRAPH_H__

#include <map>
#include <iostream>
#include <kyldr/target-span.h>
#include <kyldr/span-stack.h>
#include <kyldr/feature-vector.h>
#include <kyldr/feature-data-base.h>
#include <kyldr/hyper-edge.h>
#include <kyldr/util.h>

namespace kyldr {

class ReordererModel;
class FeatureSet;

typedef std::map<HyperEdge, FeatureVectorInt*> EdgeFeatureMap;
typedef std::pair<HyperEdge, FeatureVectorInt*> EdgeFeaturePair;


class HyperGraph {
public:
    
    friend class TestHyperGraph;

    HyperGraph() : features_(0) { }

    ~HyperGraph() {
        if(features_) {
            BOOST_FOREACH(EdgeFeaturePair efp, *features_)
                delete efp.second;
            delete features_;
        }
        BOOST_FOREACH(SpanStack * stack, stacks_)
            delete stack;
    }
    
    void BuildHyperGraph(const ReordererModel & model,
                         FeatureSet & features,
                         const Sentence & sent,
                         int beam_size = 0);

    const SpanStack * GetStack(int l, int r) const {
        return SafeAccess(stacks_, GetTrgSpanID(l,r));
    }
    SpanStack * GetStack(int l, int r) {
        return SafeAccess(stacks_, GetTrgSpanID(l,r));
    }
    const std::vector<SpanStack*> & GetStacks() const { return stacks_; }
    std::vector<SpanStack*> & GetStacks() { return stacks_; }


    // Scoring functions
    double Score(const ReordererModel & model, double loss_multiplier,
                 TargetSpan* span);
    double Score(const ReordererModel & model, double loss_multiplier,
                 Hypothesis* hyp);

    TargetSpan * GetTrgSpan(int l, int r, int rank) {
#ifdef KYLDR_SAFE
        if(l < 0 || r < 0 || rank < 0)
            THROW_ERROR("Bad GetTrgSpan (l="<<l<<", r="<<r<<")"<<std::endl);
#endif
        int idx = GetTrgSpanID(l,r);
        if((int)stacks_.size() <= idx)
            return NULL;
        else
            return SafeAccess(stacks_, idx)->GetSpanOfRank(rank);
    }

    // Rescore the hypergraph using the given model and a loss multiplier
    double Rescore(const ReordererModel & model, double loss_multiplier);

    const TargetSpan * GetRoot() const {
        return SafeAccess(stacks_, stacks_.size()-1)->GetSpanOfRank(0);
    }

    // Add up the loss over an entire subtree defined by span
    double AccumulateLoss(const TargetSpan* span);

    FeatureVectorInt AccumulateFeatures(const TargetSpan* span);

    void AccumulateFeatures(const TargetSpan* span, 
                            std::map<int,double> & feat_map);

    void SetFeatures(EdgeFeatureMap * features) { features_ = features;}
    EdgeFeatureMap * GetFeatures() { return features_; }
    // Clear the feature array without deleting the features themselves
    // This is useful if you want to save the features for later use
    void ClearFeatures() { features_ = NULL; }

protected:
    SpanStack * ProcessOneSpan(const ReordererModel & model,
                               FeatureSet & features,
                               const Sentence & sent,
                               int l, int r,
                               int beam_size = 0);


    const FeatureVectorInt * GetEdgeFeatures(
                                FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge);

    void SetEdgeFeatures(const HyperEdge & edge, FeatureVectorInt * feat) {
        if(!features_) features_ = new EdgeFeatureMap;
        features_->insert(MakePair(edge, feat));
    }

    double GetEdgeScore(const ReordererModel & model,
                        FeatureSet & feature_gen,
                        const Sentence & sent,
                        const HyperEdge & edge);

    inline int GetTrgSpanID(int l, int r) const { 
        // If l=-1, we want the root, so return the last element of stacks_
        if(l == -1) return stacks_.size() - 1;
        return r*(r+1)/2 + l;
    }

    void SetStack(int l, int r, SpanStack * stack) {
#ifdef KYLDR_SAFE
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
    std::vector<SpanStack*> stacks_;

};

}

#endif
