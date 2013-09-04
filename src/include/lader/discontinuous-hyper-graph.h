/*
 * discontinuous-hyper-graph.h
 *
 *  Created on: Apr 8, 2013
 *      Author: leona
 */

#ifndef DISCONTINUOUS_HYPER_GRAPH_H_
#define DISCONTINUOUS_HYPER_GRAPH_H_
#include <lader/hyper-graph.h>
#include <lader/hypothesis-queue.h>
#include <lader/feature-set.h>
using namespace lader;

namespace lader{

class DiscontinuousHyperGraph : public HyperGraph{
	friend class TestDiscontinuousHyperGraph;
public:
	DiscontinuousHyperGraph(int gapSize = 1, bool cube_growing = false, bool mp = false, bool full_fledged = false, int verbose = 0)
	: HyperGraph(cube_growing), gap_(gapSize), mp_(mp), full_fledged_(full_fledged), verbose_(verbose){ }
    virtual ~DiscontinuousHyperGraph(){
        BOOST_FOREACH(HyperGraph* graph, next_)
            if (graph != NULL)
                delete graph;
    }

    virtual void AddLoss(LossBase *loss, const Ranks *ranks, const FeatureDataParse *parse) const;
    virtual void GetReordering(std::vector<int> & reord, Hypothesis * hyp) const;
    virtual void AccumulateNonLocalFeatures(std::tr1::unordered_map<int,double> & feat_map,
    		ReordererModel & model, const FeatureSet & feature_gen,
    		const Sentence & sent, const Hypothesis & hyp);
    const TargetSpan *GetStack(int l, int m, int n, int r) const
    {
        if(m < 0 && n < 0)
            return HyperGraph::GetStack(l, r);

        int idx = GetTrgSpanID(l, m);
        if (mp_ && next_.size() <= idx)
			return NULL;
        HyperGraph *hyper_graph = SafeAccess(next_, idx);
        if (hyper_graph == NULL)
			return NULL;
        if (mp_ && hyper_graph->GetStacks().size() <= GetTrgSpanID(n - m - 2, r - m - 2))
			return NULL;
        return SafeAccess(hyper_graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
    }
    TargetSpan *GetStack(int l, int m, int n, int r)
    {
        if(m < 0 && n < 0)
            return HyperGraph::GetStack(l, r);

        int idx = GetTrgSpanID(l, m);
        if (mp_ && next_.size() <= idx)
			return NULL;
        HyperGraph *hyper_graph = SafeAccess(next_, idx);
        if (hyper_graph == NULL)
			return NULL;
        if (mp_ && hyper_graph->GetStacks().size() <= GetTrgSpanID(n - m - 2, r - m - 2))
			return NULL;
        return SafeAccess(hyper_graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
    }
    void SetStack(int l, int m, int n, int r, TargetSpan *stack)
    {
        if(m < 0 && n < 0){
            HyperGraph::SetStack(l, r, stack);
            return;
        }
        if(l < 0 || r < 0 || (n > 0 && m >= n-1) )
	            THROW_ERROR("Bad SetStack "
	            		"[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
        int idx = GetTrgSpanID(l, m);
        if((int)next_.size() <= idx)
                next_.resize(idx+1, NULL);
        if(next_[idx] == NULL)
        	next_[idx] = new HyperGraph;
        HyperGraph *hyper_graph = SafeAccess(next_, idx);
        idx = GetTrgSpanID(n - m - 2, r - m - 2);
        std::vector<TargetSpan*> & stacks = hyper_graph->GetStacks();
        if((int)stacks.size() <= idx)
	            stacks.resize(idx+1, NULL);
        if(stacks[idx])
            delete stacks[idx];

        stacks[idx] = stack;
    }
    void SetVerbose(int verbose) { verbose_ = verbose; }
protected:
    TargetSpan *ProcessOneSpan(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int r, int beam_size = 0);
    TargetSpan *ProcessOneDiscontinuousSpan(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int m, int n, int r, int beam_size = 0);
    void AddHypotheses(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void AddDiscontinuousHypotheses(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void StartBeamSearch(int beam_size, HypothesisQueue q, ReordererModel & model, const Sentence & sent, const FeatureSet & features, const FeatureSet & non_local_features, TargetSpan * ret, int l, int r);
private:
    int gap_;
    bool mp_;
    int verbose_;
    int full_fledged_;
    std::vector<HyperGraph*> next_;
};

}

#endif /* DISCONTINUOUS_HYPER_GRAPH_H_ */
