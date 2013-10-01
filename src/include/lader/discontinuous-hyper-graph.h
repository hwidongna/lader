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

// x + x = discontinuous
// [0, m, n, N] is meaningless since it cannot be combined
// [l, m, n, r] s.t. m-l < D && l > 0 or r-n < D && r < N-1
inline bool IsXXD(int l, int m, int n, int r, int D, int N){
	// boundary and size check
	if (r >= N || r-l >= N)
		return false;
	return (m-l < D && l > 0) || (r-n < D && r < N-1);
}

// discontinuous + discontinuous = continuous
// [l, m, n, r] s.t.
inline bool IsOnlyDDC(int l, int m, int n, int r, int D, int N){
	// boundary check
	if (r >= N)
		false;
    return (m-l >= D && r-n >= D) || (n-m > D+1 && n-m <= 2*D) || (l == 0 && r == N-1)
    		|| (r == N-1 && m-l >= D && r-n < D) || (l == 0 && m-l < D && r-n >= D);
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
					    				sent_(sent), l_(l), r_(r), beam_size_(beam_size) { }
		void Run(){
            graph_->ProcessOneSpan(model_, features_, sent_, l_, r_, beam_size_);
        }

    private:
        DiscontinuousHyperGraph *graph_;
        ReordererModel & model_;
        const FeatureSet & features_;
        const Sentence & sent_;
        int l_, r_, beam_size_;
        int gap_size_;
    };
    virtual Task *NewSpanTask(HyperGraph *graph, ReordererModel & model, const FeatureSet & features, const FeatureSet & non_local_features, const Sentence & sent, int l, int r, int beam_size)
    {
        return new DiscontinuousTargetSpanTask(this, model, features, non_local_features, sent, l, r, beam_size);
    }

    DiscontinuousHyperGraph(int gap_size, bool cube_growing = false, bool full_fledged = false, bool mp = false, int verbose = 0)
    :HyperGraph(cube_growing), gap_size_(gap_size), full_fledged_(full_fledged), mp_(mp), verbose_(verbose)
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
        HyperGraph *cloned = new DiscontinuousHyperGraph(gap_size_, cube_growing_, full_fledged_, mp_, verbose_);
        cloned->SetThreads(threads_);
        cloned->SetBeamSize(beam_size_);
        cloned->SetPopLimit(pop_limit_);
        cloned->SetSaveFeatures(save_features_);
        cloned->SetNumWords(n_);
        cloned->MarkCloned();
        return cloned;
    }

    virtual void BuildHyperGraph(ReordererModel & model, const FeatureSet & features, const Sentence & sent);
	// Add up the loss over an entire sentence
	void AddLoss(
			LossBase* loss,
			const Ranks * ranks,
			const FeatureDataParse * parse) const;

    virtual void AccumulateFeatures(std::tr1::unordered_map<int,double> & feat_map, ReordererModel & model, const FeatureSet & features, const Sentence & sent, const Hypothesis *hyp);
    
    const TargetSpan *GetStack(int l, int m, int n, int r) const
    {
        if(m < 0 && n < 0)
            return HyperGraph::GetStack(l, r);

        int idx = GetTrgSpanID(l, m);
        if (mp_ && next_.size() <= idx)
			return NULL;
        HyperGraph * graph = SafeAccess(next_, idx);
        if ( graph == NULL)
			return NULL;
        if (mp_ &&  graph->GetStacks().size() <= GetTrgSpanID(n - m - 2, r - m - 2))
			return NULL;
        return SafeAccess( graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
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
        HyperGraph * graph = SafeAccess(next_, idx);
        if (graph == NULL)
			return NULL;
        if (graph->GetStacks().size() <= GetTrgSpanID(n - m - 2, r - m - 2))
			return NULL;
        return SafeAccess(graph->GetStacks(), GetTrgSpanID(n - m - 2, r - m - 2));
    }
    
    void SetVerbose(int verbose)
    {
        verbose_ = verbose;
    }
    // IO Functions for stored features
    virtual void FeaturesToStream(std::ostream & out){
    	int N = n_;
    	int D = gap_size_;
    	for(int L = 1; L <= N; L++)
    		for(int l = 0; l <= N-L; l++){
    			int r = l+L-1;
    			HyperGraph::GetStack(l, r)->FeaturesToStream(out);
    			for(int m = l + 1;m <= r;m++){
    				for(int d = 1;d <= 2*D && m+d+1 <= r;d++)
    					if(save_features_ && IsOnlyDDC(l, m, m+d+1, r, D, N))
    						GetStack(l, m, m+d+1, r)->FeaturesToStream(out);
					for(int d = 1;d <= D;d++)
						if (IsXXD(l, m-1, m+d, r+d, D, N))
							GetStack(l, m-1, m+d, r+d)->FeaturesToStream(out);
    			}
    		}
    }

    virtual void FeaturesFromStream(std::istream & in)
    {
        int N = n_;
        int D = gap_size_;
        for(int L = 1;L <= N;L++)
            for(int l = 0;l <= N - L;l++){
                int r = l + L - 1;
                HyperGraph::GetStack(l, r)->FeaturesFromStream(in);
                for(int m = l + 1;m <= r;m++){
                	for(int d = 1;d <= 2*D && m+d+1 <= r;d++)
                		if(save_features_ && IsOnlyDDC(l, m, m+d+1, r, D, N))
                			GetStack(l, m, m+d+1, r)->FeaturesFromStream(in);
                	for(int d = 1;d <= D;d++)
                		if (IsXXD(l, m-1, m+d, r+d, D, N))
                			GetStack(l, m-1, m+d, r+d)->FeaturesFromStream(in);
                }
    		}
    }

    virtual void ClearStacks() {
    	HyperGraph::ClearStacks();
    	BOOST_FOREACH(HyperGraph * graph, next_)
    		if (graph)
    			graph->ClearStacks();
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
        HyperGraph * graph = SafeAccess(next_, idx);
        idx =  GetTrgSpanID(n - m - 2, r - m - 2);
        std::vector<TargetSpan*> & stacks =  graph->GetStacks();
        if ((int) stacks.size() <= idx)
			stacks.resize(idx + 1, NULL);
        if (stacks[idx])
			THROW_ERROR("Stack exist "
					<< "[l="<<l<<", m="<<m<<", n="<<n<<", r="<<r<<"]"<<std::endl)
        stacks[idx] = stack;
    }

    const virtual FeatureVectorInt *GetEdgeFeatures(ReordererModel & model,
			const FeatureSet & feature_gen, const Sentence & sent,
			const HyperEdge & edge);

	virtual void SaveEdgeFeatures(int l, int r, ReordererModel & model,
			const FeatureSet & features, const Sentence & sent) {
		HyperGraph::SaveEdgeFeatures(l, r, model, features, sent);
		int N = n_;
		int D = gap_size_;
		for(int m = l + 1;m <= r;m++){
			// continuous + continuous = continuous
			GetEdgeFeatures(model, features, sent,
					HyperEdge(l, m, r, HyperEdge::EDGE_STR));
			GetEdgeFeatures(model, features, sent,
					HyperEdge(l, m, r, HyperEdge::EDGE_INV));
			// discontinuous + discontinuous = continuous
			for (int c = m+1; c-m <= D; c++){
				for(int n = c+1; n-c <= D && n <= r;n++){
					GetEdgeFeatures(model, features, sent,
							DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_STR));
					GetEdgeFeatures(model, features, sent,
							DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV));
				}
				// x + x = discontinuous
				for (int d = 1 ; d <= D ; d++)
					if ( IsXXD(l, m-1, m+d, r+d, D, N) ){
						GetEdgeFeatures(model, features, sent,
								DiscontinuousHyperEdge(l, m-1, -1, m+d, r+d, HyperEdge::EDGE_STR));
						GetEdgeFeatures(model, features, sent,
								DiscontinuousHyperEdge(l, m-1, -1, m+d, r+d, HyperEdge::EDGE_INV));
						if (full_fledged_){
							for (int c = l+1 ; c <= m ; c++){
								GetEdgeFeatures(model, features, sent,
										DiscontinuousHyperEdge(l, m-1, c, m+d, r+d, HyperEdge::EDGE_STR));
								GetEdgeFeatures(model, features, sent,
										DiscontinuousHyperEdge(l, m-1, c, m+d, r+d, HyperEdge::EDGE_INV));
							}
							for (int c = m+d+1 ; c <= r ; c++){
								GetEdgeFeatures(model, features, sent,
										DiscontinuousHyperEdge(l, m-1, c, m+d, r+d, HyperEdge::EDGE_STR));
								GetEdgeFeatures(model, features, sent,
										DiscontinuousHyperEdge(l, m-1, c, m+d, r+d, HyperEdge::EDGE_INV));
							}
						}
					}
			}
		}
	}
	// For cube pruning/growing with threads > 1, we need to set same-sized stacks
	// in advance to ProcessOneSpan
    virtual void SetStacks(int l, int r, bool init = false) {
    	HyperGraph::SetStacks(l, r, init);
    	int N = n_;
    	int D = gap_size_;
    	for(int m = l + 1;m <= r;m++){
    		for(int d = 1;d <= 2*D && m+d+1 <= r;d++)
    			if(save_features_ && IsOnlyDDC(l, m, m+d+1, r, D, N))
    				// if -save_features, stack exist
    				if (init || !GetStack(l, m, m+d+1, r) ){
    					if (verbose_ > 1)
    						cerr << "SetStack DDC [" << l << ", " << m << ", " << m+d+1 << ", " << r << "]" << endl;
    					SetStack(l, m, m+d+1, r, new DiscontinuousTargetSpan(l, m, m+d+1, r));
    				}
    		for(int d = 1;d <= D;d++)
    			if (IsXXD(l, m-1, m+d, r+d, D, N))
    				// if -save_features, stack exist
    				if(init || !GetStack(l, m-1, m+d, r+d)){
    					if (verbose_ > 1)
    						cerr << "SetStack XXD [" << l << ", " << m-1 << ", " << m+d << ", " << r+d << "]" << endl;
    					SetStack(l, m-1, m+d, r+d, new DiscontinuousTargetSpan(l, m-1, m+d, r+d));
    				}
    	}
    }
    virtual void TriggerTheBestHypotheses(int l, int r, ReordererModel & model, const Sentence & sent);
    virtual Hypothesis *LazyKthBest(TargetSpan *stack, int k, ReordererModel & model, const Sentence & sent, int & pop_count);
    void ProcessOneSpan(ReordererModel & model, const FeatureSet & features, const Sentence & sent, int l, int r, int beam_size = 0);
    void ProcessOneDiscontinuousSpan(ReordererModel & model, const FeatureSet & features, const Sentence & sent, int l, int m, int n, int r, int beam_size = 0);
    void AddHypotheses(ReordererModel & model, const FeatureSet & features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void AddDiscontinuousHypotheses(ReordererModel & model, const FeatureSet & features, const Sentence & sent, HypothesisQueue & q, int left_l, int left_m, int left_n, int left_r, int right_l, int right_m, int right_n, int right_r);
    void StartBeamSearch(int beam_size, HypothesisQueue & q, ReordererModel & model, const Sentence & sent, const FeatureSet & features, TargetSpan *ret, int l, int r);
    bool CanSkip(Hypothesis * hyp);

private:
	int gap_size_; // maximum gap size
	bool mp_; // monotone at punctuation
	int verbose_; // verbosity during process
    int full_fledged_;
	std::vector<HyperGraph*> next_;
};

}

#endif /* DISCONTINUOUS_HYPER_GRAPH_H_ */
