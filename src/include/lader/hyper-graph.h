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
#include "lm/model.hh"

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

    HyperGraph(bool cube_growing = false) : features_(0), n_(-1), cube_growing_(cube_growing), bigram_(0){ }

    void Clear()
    {
        if(features_) {
            BOOST_FOREACH(EdgeFeaturePair efp, *features_){
            	delete efp.first;
                delete efp.second;
            }
            delete features_;
            ClearFeatures();
        }
        BOOST_FOREACH(TargetSpan * stack, stacks_)
            delete stack;
        stacks_.clear();
    }

    virtual ~HyperGraph()
    {
        Clear();
        if (bigram_)
        	delete bigram_;
    }
    void BuildHyperGraph(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int beam_size = 0);
    const TargetSpan *GetStack(int l, int r) const
    {
        return SafeAccess(stacks_, GetTrgSpanID(l, r));
    }

    TargetSpan *GetStack(int l, int r)
    {
        return SafeAccess(stacks_, GetTrgSpanID(l, r));
    }

    const std::vector<TargetSpan*> & GetStacks() const
    {
        return stacks_;
    }

    std::vector<TargetSpan*> & GetStacks()
    {
        return stacks_;
    }

    double Score(double loss_multiplier, const Hypothesis *hyp) const;
    void PrintHyperGraph(const std::vector<std::string> & sent, std::ostream & out);
    double Rescore(double loss_multiplier);
    const TargetSpan *GetRoot() const
    {
        return SafeAccess(stacks_, stacks_.size() - 1);
    }

    TargetSpan *GetRoot()
    {
        return SafeAccess(stacks_, stacks_.size() - 1);
    }

    Hypothesis *GetBest()
    {
        return SafeAccess(stacks_, stacks_.size() - 1)->GetHypothesis(0);
    }

    virtual void AddLoss(LossBase *loss, const Ranks *ranks, const FeatureDataParse *parse) const;
    void SetFeatures(EdgeFeatureMap *features)
    {
        features_ = features;
    }

    EdgeFeatureMap *GetFeatures()
    {
        return features_;
    }

    void ClearFeatures()
    {
        features_ = NULL;
    }

    virtual void AccumulateNonLocalFeatures(std::tr1::unordered_map<int,double> & feat_map, ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const Hypothesis & hyp);
    void LoadLM(const char *file)
    {
        bigram_ = new lm::ngram::Model(file);
    }

protected:
    virtual TargetSpan *ProcessOneSpan(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int r, int beam_size = 0);
    const FeatureVectorInt *GetEdgeFeatures(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const HyperEdge & edge);
    void SetEdgeFeatures(const HyperEdge & edge, FeatureVectorInt *feat)
    {
        if(!features_)
            features_ = new EdgeFeatureMap;

        features_->insert(MakePair(edge.Clone(), feat));
    }
    double GetEdgeScore(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const HyperEdge & edge);
    double GetNonLocalScore(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const Hypothesis & left, const Hypothesis & right);
    inline int GetTrgSpanID(int l, int r) const
    {
        if(l == -1)
            return stacks_.size() - 1;

        return r * (r + 1) / 2 + l;
    }
    virtual void SetStack(int l, int r, TargetSpan *stack)
    {
        if(l < 0 || r < 0)
            THROW_ERROR("Bad SetStack (l="<<l<<", r="<<r<<")"<<std::endl)
        ;
        int idx = HyperGraph::GetTrgSpanID(l, r);
        if((int)stacks_.size() <= idx)
            stacks_.resize(idx+1, NULL);
        if(stacks_[idx])
            delete stacks_[idx];

        stacks_[idx] = stack;
    }
private:
    EdgeFeatureMap *features_;
    std::vector<TargetSpan*> stacks_;
protected:
    void AddTerminals(int l, int r, const FeatureSet & features, ReordererModel & model, const Sentence & sent, HypothesisQueue *& q);
    int n_;
    bool cube_growing_;
    lm::ngram::Model * bigram_;
};

}

#endif

