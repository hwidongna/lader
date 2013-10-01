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
#include <lader/thread-pool.h>

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
    class TargetSpanTask : public Task {
    public:
    	TargetSpanTask(HyperGraph * graph, ReordererModel & model,
    			const FeatureSet & features,
    			const Sentence & sent, int l, int r, int beam_size) :
    				graph_(graph), model_(model), features_(features),
    				sent_(sent), l_(l), r_(r), beam_size_(beam_size) { }
    	void Run(){
    		graph_->ProcessOneSpan(model_, features_,
    		    						sent_, l_, r_, beam_size_);
    	}
    private:
    	HyperGraph * graph_;
    	ReordererModel & model_;
    	const FeatureSet & features_;
    	const Sentence & sent_;
    	int l_, r_, beam_size_;
    };

    virtual Task * NewSpanTask(HyperGraph * graph, ReordererModel & model,
			const FeatureSet & features,
			const Sentence & sent, int l, int r, int beam_size){
    	return new TargetSpanTask(this, model, features,
				sent, l, r, beam_size);
    }
    HyperGraph(bool cube_growing = false) :
			save_features_(false), n_(-1), threads_(1), cube_growing_(cube_growing),
			cloned_(false), beam_size_(0), pop_limit_(0) { }

    virtual void ClearStacks() {
		BOOST_FOREACH(TargetSpan * stack, stacks_)
			delete stack;
		stacks_.clear();
	}

    virtual ~HyperGraph()
    {
        ClearStacks();
    }

    virtual HyperGraph * Clone(){
    	HyperGraph * cloned = new HyperGraph(cube_growing_);
    	cloned->SetThreads(threads_);
    	cloned->SetBeamSize(beam_size_);
    	cloned->SetPopLimit(pop_limit_);
    	cloned->SetSaveFeatures(save_features_);
    	cloned->SetNumWords(n_);
    	cloned->MarkCloned();
    	return cloned;
    }

    // Build the hypergraph using the specified model, features and sentence
    //  beam_size: the pop limit for cube pruning
    virtual void BuildHyperGraph(ReordererModel & model,
			const FeatureSet & features,
			const Sentence & sent);
	const TargetSpan *GetStack(int l, int r) const {
		return SafeAccess(stacks_, GetTrgSpanID(l, r));
	}

	TargetSpan *GetStack(int l, int r) {
		return SafeAccess(stacks_, GetTrgSpanID(l, r));
	}

	const std::vector<TargetSpan*> & GetStacks() const {
		return stacks_;
	}

	std::vector<TargetSpan*> & GetStacks() {
		return stacks_;
	}
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
  		if (stacks_.empty())
			return NULL;
    	return SafeAccess(stacks_, stacks_.size()-1)->GetHypothesis(0);
    }
    // Add up the loss over an entire sentence
    virtual void AddLoss(
    		LossBase* loss,
    		const Ranks * ranks,
            const FeatureDataParse * parse) const;

    void SetSaveFeatures(bool save_features) {
    	save_features_ = save_features;
    }
	virtual void AccumulateFeatures(std::tr1::unordered_map<int, double> & feat_map,
			ReordererModel & model, const FeatureSet & features,
			const Sentence & sent,
			const Hypothesis * hyp);

	void SetThreads(int threads) {
		threads_ = threads;
	}
    void SetBeamSize(int beam_size) {
        beam_size_ = beam_size;
    }
    void SetPopLimit(int pop_limit) {
    	pop_limit_ = pop_limit;
    }

    void MarkCloned(){ cloned_ = true; }
    void SetNumWords(int n){ n_ = n;}
	// IO Functions for stored features
	virtual void FeaturesToStream(std::ostream & out){
		BOOST_FOREACH(TargetSpan * stack, stacks_)
			stack->FeaturesToStream(out);
	}
	virtual void FeaturesFromStream(std::istream & in){
		BOOST_FOREACH(TargetSpan * stack, stacks_)
			stack->FeaturesFromStream(in);
	}

	void SetAllStacks(){
		stacks_.resize(n_ * (n_+1) / 2 + 1, NULL); // resize stacks in advance
		for(int L = 1; L <= n_; L++)
			for(int l = 0; l <= n_-L; l++)
				SetStacks(l, l+L-1, true);
		// Set root stack at the end of the list
		TargetSpan * root_stack = new TargetSpan(0,n_);
		stacks_[n_ * (n_+1) / 2] = root_stack;
	}

	void SaveAllEdgeFeatures(ReordererModel & model, const FeatureSet & features,
			const Sentence & sent) {
		for(int L = 1; L <= n_; L++)
			for(int l = 0; l <= n_-L; l++)
				SaveEdgeFeatures(l, l+L-1, model, features, sent);
	}
protected:
    void AddTerminals(int l, int r, const FeatureSet & features,
			ReordererModel & model, const Sentence & sent,
			HypothesisQueue *& q);
    // For both cube pruning/growing
    void IncrementLeft(const Hypothesis *old_hyp, const Hypothesis *new_left,
			ReordererModel & model,
			const Sentence & sent, HypothesisQueue & q, int & pop_count);
	void IncrementRight(const Hypothesis *old_hyp, const Hypothesis *new_right,
			ReordererModel & model,
			const Sentence & sent, HypothesisQueue & q, int & pop_count);

	virtual void SaveEdgeFeatures(int l, int r, ReordererModel & model,
			const FeatureSet & features, const Sentence & sent) {
		for(int c = l+1; c <= r; c++) {
			GetEdgeFeatures(model, features, sent,
					HyperEdge(l, c, r, HyperEdge::EDGE_STR));
			GetEdgeFeatures(model, features, sent,
					HyperEdge(l, c, r, HyperEdge::EDGE_INV));
		}
	}
	// For cube pruning/growing with threads > 1, we need to set same-sized stacks
	// in advance to ProcessOneSpan
	virtual void SetStacks(int l, int r, bool init = false) {
		// if -save_features, stack exist
		if (init || !GetStack(l, r))
			SetStack(l, r, new TargetSpan(l, r));
	}
	// For cube growing with threads > 1, we need to trigger the best hypothesis
	// in advance to LazyKthBest
	virtual void TriggerTheBestHypotheses(int l, int r, ReordererModel & model,
			const Sentence & sent);
	// For cube growing
	virtual void LazyNext(HypothesisQueue & q, ReordererModel & model,
			const Sentence & sent,
			const Hypothesis * hyp, int & pop_count);
    virtual Hypothesis * LazyKthBest(TargetSpan * stack, int k,
			ReordererModel & model,
			const Sentence & sent, int & pop_count);
    virtual void ProcessOneSpan(ReordererModel & model,
    		const FeatureSet & features,
			const Sentence & sent, int l, int r, int beam_size);
    const virtual FeatureVectorInt *GetEdgeFeatures(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const HyperEdge & edge);
    double GetEdgeScore(ReordererModel & model, const FeatureSet & feature_gen, const Sentence & sent, const HyperEdge & edge);
    inline int GetTrgSpanID(int l, int r) const
    {
        // If l=-1, we want the root, so return the last element of stacks_
        if(l == -1)
            return stacks_.size() - 1;

        return r * (r + 1) / 2 + l;
    }

    void SetStack(int l, int r, TargetSpan *stack)
    {
        if(l < 0 || r < 0)
            THROW_ERROR("Bad SetStack [l="<<l<<", r="<<r<<"]"<<std::endl)
        int idx = GetTrgSpanID(l, r);
        if((int)stacks_.size() <= idx)
			stacks_.resize(idx+1, NULL);
        if(stacks_[idx])
        	THROW_ERROR("Stack exist [l="<<l<<", r="<<r<<"]"<<std::endl)
        stacks_[idx] = stack;
    }
    
private:
    // Stacks containing the hypotheses for each span
    // The indexing for the outer vector is:
    //  0-0 -> 0, 0-1 -> 1, 1-1 -> 2, 0-2 -> 3 ...
    // And can be recovered by GetHypothesis.
    // The inner vector contains target spans in descending rank of score
    std::vector<TargetSpan*> stacks_;

protected:
    int pop_limit_;
    // the beam size used for cube pruning/growing
    int beam_size_;
    bool save_features_;
    int threads_;
    // The length of the sentence
    int n_;
    bool cube_growing_, cloned_;
    boost::mutex mutex_;
};

}

#endif

