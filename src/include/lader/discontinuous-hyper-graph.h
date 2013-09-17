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
#include <lader/discontinuous-target-span.h>
#include <boost/thread/mutex.hpp>
using namespace lader;

namespace lader{

// Process [0, m, n, N] is meaningless
// Process [l, m, n, r] s.t. m-l < D && l > 0 or r-n < D && r < N-1
inline bool IsMeaningful(int l, int m, int n, int r, int D, int N){
	return r < N && r-l+1 < N && ((m-l < D && l > 0) || (r-n < D && r < N-1));
}

class DiscontinuousHyperGraph : public HyperGraph{
public:
	friend class TestDiscontinuousHyperGraph;
	class DiscontinuousTargetSpanTask : public Task {
	public:
		DiscontinuousTargetSpanTask(DiscontinuousHyperGraph * graph, ReordererModel & model,
				const FeatureSet & features, const FeatureSet & non_local_features,
				const Sentence & sent, int l, int r, int beam_size) :
					graph_(graph), model_(model), features_(features),
					    				non_local_features_(non_local_features), sent_(sent), l_(l), r_(r), beam_size_(beam_size) { }
		void Run(){
            graph_->ProcessOneSpan(model_, features_, non_local_features_, sent_, l_, r_, beam_size_);
        }

    private:
        DiscontinuousHyperGraph *graph_;
        ReordererModel & model_;
        const FeatureSet & features_;
        const FeatureSet & non_local_features_;
        const Sentence & sent_;
        int l_, r_, beam_size_;
        int gap_size_;
    };
    virtual Task *NewSpanTask(HyperGraph *graph, ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int r, int beam_size)
    {
        return new DiscontinuousTargetSpanTask(this, model, features, non_local_features, sent, l, r, beam_size);
    }

    DiscontinuousHyperGraph(int gap_size, int max_seq = 1, bool cube_growing = false, bool full_fledged = false, bool mp = false, int verbose = 0)
    :HyperGraph(cube_growing), gap_size_(gap_size), max_seq_(max_seq), full_fledged_(full_fledged), mp_(mp), verbose_(verbose)
    {
    }

    virtual ~DiscontinuousHyperGraph()
    {
        BOOST_FOREACH(HyperGraph* graph, next_)
            if (graph != NULL)
                delete graph;
    }

    virtual HyperGraph *Clone()
    {
        HyperGraph *cloned = new DiscontinuousHyperGraph(gap_size_, max_seq_, cube_growing_, full_fledged_, mp_, verbose_);
        cloned->SetThreads(threads_);
        cloned->SetBeamSize(beam_size_);
        cloned->SetPopLimit(pop_limit_);
        cloned->SaveFeatures(save_features_);
        cloned->MarkCloned();
        cloned->SetLM(bigram_);
        return cloned;
    }

    virtual void BuildHyperGraph(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent);
    virtual void AddLoss(LossBase *loss, const Ranks *ranks, const FeatureDataParse *parse) const;
    virtual void AccumulateFeatures(std::tr1::unordered_map<int,double> & feat_map, ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, const Hypothesis *hyp);
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

        if(l < 0 || r < 0 || (n > 0 && m >= n-1) )
	            THROW_ERROR("Bad GetStack " <<
	            		"[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
        int idx = GetTrgSpanID(l, m);
        if (next_.size() <= idx)
			return NULL;
        HyperGraph *hyper_graph = SafeAccess(next_, idx);
        if (hyper_graph == NULL)
			return NULL;
        if (hyper_graph->GetStacks().size() <= GetTrgSpanID(n - m - 2, r - m - 2))
			return NULL;
        return SafeAccess(hyper_graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
    }
    void SetVerbose(int verbose)
    {
        verbose_ = verbose;
    }

protected:
    void SetStack(int l, int m, int n, int r, TargetSpan *stack)
    {
        if(m < 0 && n < 0){
            HyperGraph::SetStack(l, r, stack);
            return;
        }
        if (l < 0 || r < 0 || (n > 0 && m >= n - 1))
			THROW_ERROR("Bad SetStack "
					<< "[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
        int idx = GetTrgSpanID(l, m);
        if ((int) next_.size() <= idx)
			next_.resize(idx + 1, NULL);
        if (next_[idx] == NULL)
			next_[idx] = new HyperGraph;
        HyperGraph *hyper_graph = SafeAccess(next_, idx);
        idx = GetTrgSpanID(n - m - 2, r - m - 2);
        std::vector<TargetSpan*> & stacks = hyper_graph->GetStacks();
        if ((int) stacks.size() <= idx)
			stacks.resize(idx + 1, NULL);
        if (stacks[idx])
			THROW_ERROR("Stack exist "
					<< "[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
        ;
        stacks[idx] = stack;
    }

    const virtual FeatureVectorInt *GetEdgeFeatures(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const HyperEdge & edge);
    virtual void SetStacks(int l, int r) {
    	int N = n_;
    	int D = gap_size_;
    	if(!HyperGraph::GetStack(l, r))
    		HyperGraph::SetStack(l, r, new TargetSpan(l, r));

    	for(int c = l + 1;c <= r;c++)
    		for(int d = 1;d <= D;d++){
    			for(int n = c + d + 1;n <= r && n - (c + d) <= D;n++)
    				if(save_features_ && !GetStack(l, c, n, r))
    					SetStack(l, c, n, r, new DiscontinuousTargetSpan(l, c, n, r));


    			if(IsMeaningful(l, c - 1, c + d, r + d, D, N))
    				if(!GetStack(l, c - 1, c + d, r + d))
    					SetStack(l, c - 1, c + d, r + d, new DiscontinuousTargetSpan(l, c - 1, c + d, r + d));
    		}
    }
    virtual void LazyNext(HypothesisQueue & q, ReordererModel & model, const FeatureSet & non_local_features, const Sentence & sent, const Hypothesis *hyp, int & pop_count);
    virtual Hypothesis *LazyKthBest(TargetSpan *stack, int k, ReordererModel & model, const FeatureSet & non_local_features, const Sentence & sent, int pop_count = 0);
    void ProcessOneSpan(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int r, int beam_size = 0);
    void ProcessOneDiscontinuousSpan(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int m, int n, int r, int beam_size = 0);
    void AddHypotheses(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void AddDiscontinuousHypotheses(ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void StartBeamSearch(int beam_size, HypothesisQueue & q, ReordererModel & model, const Sentence & sent, const FeatureSet & features, const FeatureSet & non_local_features, TargetSpan *ret, int l, int r);
    bool CanSkip(Hypothesis * hyp);

private:
    int gap_size_;
    int max_seq_;
    bool mp_;
    int verbose_;
    int full_fledged_;
    std::vector<HyperGraph*> next_;
};

}

#endif /* DISCONTINUOUS_HYPER_GRAPH_H_ */
