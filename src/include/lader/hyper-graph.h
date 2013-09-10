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
    friend class Hypothesis;

    HyperGraph(bool cube_growing = false):
    	feature_map_(0), n_(-1), cube_growing_(cube_growing), cloned_(false), bigram_(0){ }

    virtual void Clear()
    {
        if(feature_map_) {
            BOOST_FOREACH(EdgeFeaturePair efp, *feature_map_){
            	delete efp.first;
                delete efp.second;
            }
            delete feature_map_;
            ClearFeatures();
        }
        BOOST_FOREACH(TargetSpan * stack, stacks_)
            delete stack;
        stacks_.clear();
    }

    virtual ~HyperGraph()
    {
        Clear();
        if (!cloned_ && bigram_)
        	delete bigram_;
    }

    HyperGraph * Clone(){
    	HyperGraph * cloned = new HyperGraph(this);
    	cloned->cloned_ = true;
    	return cloned;
    }
    // Generate all edge features and store them
    virtual void StoreEdgeFeatures(HyperEdge * edge, ReordererModel & model,
			const FeatureSet & features, const Sentence & sent);
	virtual void GenerateEdgeFeatures(ReordererModel & model,
			const FeatureSet & features, const Sentence & sent);

    void BuildHyperGraph(ReordererModel & model, const FeatureSet & features,
			const FeatureSet & non_local_features, const Sentence & sent,
			int beam_size = 0);
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
        feature_map_ = features;
    }

    EdgeFeatureMap *GetFeatures()
    {
        return feature_map_;
    }

    void ClearFeatures()
    {
        feature_map_ = NULL;
    }

    virtual void AccumulateNonLocalFeatures(
			std::tr1::unordered_map<int, double> & feat_map
			, ReordererModel & model, const FeatureSet & feature_gen
			, const Sentence & sent, const Hypothesis * hyp);
    void LoadLM(const char *file)
    {
        bigram_ = new lm::ngram::Model(file);
    }

protected:
    // For both cube pruning/growing
    void IncrementLeft(const Hypothesis *old_hyp, const Hypothesis *new_left,
			ReordererModel & model, const FeatureSet & non_local_features,
			const Sentence & sent, HypothesisQueue & q);
	void IncrementRight(const Hypothesis *old_hyp, const Hypothesis *new_right,
			ReordererModel & model, const FeatureSet & non_local_features,
			const Sentence & sent, HypothesisQueue & q);

	// For cube growing
	virtual void LazyNext(HypothesisQueue & q, ReordererModel & model,
			const FeatureSet & features, const FeatureSet & non_local_features,
			const Sentence & sent, const Hypothesis * hyp);
    virtual Hypothesis * LazyKthBest(TargetSpan * stack, int k,
			ReordererModel & model, const FeatureSet & features,
			const FeatureSet & non_local_features, const Sentence & sent);
    virtual TargetSpan *ProcessOneSpan(ReordererModel & model,
			const FeatureSet & features, const FeatureSet & non_local_features,
			const Sentence & sent, int l, int r, int beam_size = 0);
	const FeatureVectorInt *GetEdgeFeatures(const HyperEdge & edge);
	// only for testing
    void SetEdgeFeatures(const HyperEdge & edge, FeatureVectorInt *feat)
    {
        if(!feature_map_)
            feature_map_ = new EdgeFeatureMap;

        feature_map_->insert(MakePair(edge.Clone(), feat));
    }
    double GetEdgeScore(const ReordererModel & model, const HyperEdge & edge);
    const FeatureVectorInt *GetNonLocalFeatures(const HyperEdge * edge,
    		const Hypothesis *left, const Hypothesis *right, const FeatureSet & feature_gen,
    		const Sentence & sent, ReordererModel & model, lm::ngram::State * out);
    double GetNonLocalScore(ReordererModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			const HyperEdge *edge, const Hypothesis *left,	const Hypothesis *right,
			lm::ngram::State *out = NULL);
    double GetRootBigram(const Sentence & sent, const Hypothesis *hyp,
			lm::ngram::Model::State * out);

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
    std::vector<TargetSpan*> stacks_;

protected:
    void AddTerminals(int l, int r, const FeatureSet & features,
			ReordererModel & model, const Sentence & sent,
			HypothesisQueue *& q);
    EdgeFeatureMap *feature_map_;
    int n_;
    bool cube_growing_, cloned_;
    lm::ngram::Model * bigram_;
};

}

#endif

