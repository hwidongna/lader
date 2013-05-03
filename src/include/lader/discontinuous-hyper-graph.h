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
	DiscontinuousHyperGraph(int gapSize = 1, bool mp = false, int verbose = 0)
	: gap_(gapSize), mp_(mp), verbose_(verbose){ }
    virtual ~DiscontinuousHyperGraph(){
        BOOST_FOREACH(HyperGraph* graph, next_)
            if (graph != NULL)
                delete graph;
    }

	// Add up the loss over an entire sentence
	void AddLoss(
			LossBase* loss,
			const Ranks * ranks,
			const FeatureDataParse * parse) const;

	// Rescore the hypergraph using the given model and a loss multiplier
	double Rescore(const ReordererModel & model, double loss_multiplier);

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

//	Hypothesis *GetTrgSpan(int l, int m, int n, int r, int rank)
//	{
//		if (m < 0 && n < 0)
//			return HyperGraph::GetSpanHypothesis(l, r, rank);
//		if(l < 0 || r < 0 || (n > 0 && m >= n-1) || rank < 0)
//			THROW_ERROR("Bad GetTrgSpan "<<rank<<"th of "
//					"[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
//		HyperGraph *hyper_graph = SafeAccess(next_, GetTrgSpanID(l, m));
//		int idx = GetTrgSpanID(n - m - 2, r - m - 2);
//	    std::vector<TargetSpan*> & stacks = hyper_graph->GetStacks();
//		if((int)stacks.size() <= idx)
//			return NULL;
//		else
//			return SafeAccess(stacks, idx)->GetHypothesis(rank);
//	}
	void SetStack(int l, int m, int n, int r, TargetSpan * stack) {
		if (m < 0 && n < 0){
			HyperGraph::SetStack(l, r , stack);
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
protected:
    TargetSpan *ProcessOneSpan(
    		ReordererModel & model,
    		const FeatureSet & features,
    		const Sentence & sent,
    		int l, int r,
    		int beam_size = 0, bool save_trg = true);
    TargetSpan *ProcessOneDiscontinuousSpan(
    		ReordererModel & model,
    		const FeatureSet & features,
    		const Sentence & sent,
    		int l, int m, int n, int r,
    		int beam_size = 0, bool save_trg = true);
    void AddHyperEdges(
    		ReordererModel & model,	const FeatureSet & features,
    		const Sentence & sent, HypothesisQueue & q,
    		int left_l, int left_m, int left_n, int left_r,
    		int right_l, int right_m, int right_n, int right_r);
    void AddDiscontinuousHyperEdges(
    		ReordererModel & model, const FeatureSet & features,
    		const Sentence & sent, HypothesisQueue & q,
    		int left_l, int left_m, int left_n, int left_r,
    		int right_l, int right_m, int right_n, int right_r);
    void nextCubeItems(const Hypothesis * hyp,
    		HypothesisQueue & q, int l, int r, bool gap=true);
private:
	int gap_; // maximum gap size
	bool mp_; // monotone at punctuation
	int verbose_; // verbosity during process
	std::vector<HyperGraph*> next_;
};

}

#endif /* DISCONTINUOUS_HYPER_GRAPH_H_ */
