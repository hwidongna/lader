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
	DiscontinuousHyperGraph(int gapSize = 1)
	: gap_(gapSize) { }

	void BuildHyperGraph(ReordererModel & model, const FeatureSet & features, const Sentence & sent, int beam_size = 0, bool save_trg = true);
	SpanStack *GetStack(int l, int m, int n, int r)
	{
		if(m < 0 && n < 0)
			return HyperGraph::GetStack(l, r);

		HyperGraph *hyper_graph = SafeAccess(next_, GetTrgSpanID(l, m));
		if (hyper_graph == NULL){
			return NULL;
		}
		return SafeAccess(hyper_graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
	}

	TargetSpan *GetTrgSpan(int l, int m, int n, int r, int rank)
	{
		if (m < 0 && n < 0)
			return HyperGraph::GetTrgSpan(l, r, rank);
		if(l < 0 || r < 0 || (n > 0 && m >= n-1) || rank < 0)
			THROW_ERROR("Bad GetTrgSpan "<<rank<<"th of "
					"[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
		HyperGraph *hyper_graph = SafeAccess(next_, GetTrgSpanID(l, m));
		int idx = GetTrgSpanID(n - m - 2, r - m - 2);
	    std::vector<SpanStack*> & stacks = hyper_graph->GetStacks();
		if((int)stacks.size() <= idx)
			return NULL;
		else
			return SafeAccess(stacks, idx)->GetSpanOfRank(rank);
	}
	void SetStack(int l, int m, int n, int r, SpanStack * stack) {
		if (m < 0 && n < 0){
			HyperGraph::SetStack(l, r , stack);
			return;
		}
	#ifdef LADER_SAFE
			if(l < 0 || r < 0 || (n > 0 && m >= n-1) )
	            THROW_ERROR("Bad SetStack "
	            		"[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
	#endif
			HyperGraph *hyper_graph = SafeAccess(next_, GetTrgSpanID(l, m));
	        int idx = GetTrgSpanID(n - m - 2, r - m - 2);
	        std::vector<SpanStack*> & stacks = hyper_graph->GetStacks();
	        if((int)stacks.size() <= idx)
	            stacks.resize(idx+1, NULL);
	        if(stacks[idx]) delete stacks[idx];
	        stacks[idx] = stack;
	    }
protected:
	SpanStack *ProcessOneSpan(
			ReordererModel & model,
			const FeatureSet & features,
			const Sentence & sent,
			int l, int r,
			int beam_size = 0,
			bool save_trg = true);
	SpanStack *ProcessOneDiscontinuousSpan(
				ReordererModel & model,
				const FeatureSet & features,
				const Sentence & sent,
				int l, int m, int n, int r,
				int beam_size = 0,
				bool save_trg = true);
    void AddHyperEdges(ReordererModel & model, const FeatureSet & features,
			const Sentence & sent,	HypothesisQueue & q,
			int left_l, int left_m, int left_n, int left_r,
			int right_l, int right_m, int right_n, int right_r);
    void AddDiscontinuousHyperEdges(ReordererModel & model, const FeatureSet & features,
    			const Sentence & sent,	HypothesisQueue & q,
    			int left_l, int left_m, int left_n, int left_r,
    			int right_l, int right_m, int right_n, int right_r);
private:
	int gap_;
	std::vector<HyperGraph*> next_;
};

}

#endif /* DISCONTINUOUS_HYPER_GRAPH_H_ */
