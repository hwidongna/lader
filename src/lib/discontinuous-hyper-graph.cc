/*
 * discontinuous-hyper-graph.cc
 *
 *  Created on: Apr 8, 2013
 *      Author: leona
 */
#include <lader/hyper-graph.h>
#include <lader/discontinuous-hyper-graph.h>
#include <lader/discontinuous-hypothesis.h>
#include <lader/discontinuous-hyper-edge.h>
#include <lader/hypothesis-queue.h>
#include <lader/reorderer-model.h>
#include <lader/feature-vector.h>
#include <tr1/unordered_map>
#include <boost/algorithm/string.hpp>
#include <lader/util.h>
#include <typeinfo>

using namespace lader;
using namespace std;
using namespace boost;

// Return the edge feature vector
// this is called only once from HyperGraph::GetEdgeScore
// therefore, we don't need to insert and find features in stack
// unless we use -save_features option
const FeatureVectorInt * DiscontinuousHyperGraph::GetEdgeFeatures(
                                ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge) {
	FeatureVectorInt * ret;
	const DiscontinuousHyperEdge * dedge =
			dynamic_cast<const DiscontinuousHyperEdge *>(&edge);
    if (save_features_) {
    	TargetSpan * stack;
		if (dedge){
			stack = GetStack(edge.GetLeft(), dedge->GetM(), dedge->GetN(), dedge->GetRight());
			if (!stack)
				THROW_ERROR("SetStack first: " << *dedge << endl)
		}
		else{
			stack = HyperGraph::GetStack(edge.GetLeft(), edge.GetRight());
			if (!stack)
				THROW_ERROR("SetStack first: " << edge << endl)
		}
    	switch (edge.GetType()){
    	case HyperEdge::EDGE_FOR:
    	case HyperEdge::EDGE_STR:
			ret = stack->GetStraightFeautures(edge.GetCenter() - edge.GetLeft());
			if (ret == NULL){
				ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
				stack->SaveStraightFeautures(edge.GetCenter() - edge.GetLeft(), ret);
			}
			break;
    	case HyperEdge::EDGE_BAC:
    	case HyperEdge::EDGE_INV:
    		ret = stack->GetInvertedFeautures(edge.GetCenter() - edge.GetLeft());
    		if (ret == NULL){
    			ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
    			stack->SaveInvertedFeautures(edge.GetCenter() - edge.GetLeft(), ret);
    		}
    		break;
    	case HyperEdge::EDGE_ROOT:
    	default:
    		THROW_ERROR("Invalid hyper edge for GetEdgeFeatures: " << edge);
    	}
    } else {
    	// no insert, make -threads faster without -save_features
    	ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
    }
    return ret;
}

void DiscontinuousHyperGraph::AddHypotheses(
		ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent,	HypothesisQueue & q,
		int left_l, int left_m, int left_n, int left_r,
		int right_l, int right_m, int right_n, int right_r)
{
	TargetSpan * left_stack, * right_stack;
	double score, viterbi_score, non_local_score;
	// Find the best hypotheses on the left and right side
	left_stack = GetStack(left_l, left_m, left_n, left_r);
	right_stack = GetStack(right_l, right_m, right_n, right_r);
	if (!left_stack)
		THROW_ERROR("SetStack first [" << left_l << ", " << left_m << ", " << left_n << ", " << left_r << "]" << endl)
	if (!right_stack)
		THROW_ERROR("SetStack first [" << right_l << ", " << right_m << ", " << right_n << ", " << right_r << "]" << endl)
	int l, m, c, n, r;
	HyperEdge * edge;
    Hypothesis * left, *right;
   	left = left_stack->GetHypothesis(0);
   	right = right_stack->GetHypothesis(0);
	// continuous + continuous = continuous
	if (left_m < 0 && left_n < 0 && right_m < 0 && right_n < 0 && left_r+1 == right_l){
		l = left_l;
		m = -1;
		c = right_l;
		n = -1;
		r = right_r;
	}
	// discontinuous + discontinuous = continuous
	else if (left_m+1 == right_l && right_m+1 == left_n && left_r+1 == right_n){
		l = left_l;
		m = right_l;
		c = left_n;
		n = right_n;
		r = right_r;
	}
	if (l >= c || c > r)
		THROW_ERROR("Invalid Target Span ["<<l<<", "<<c<<", "<<r<<"]");
	// Add the straight terminal
	if (m < 0 && n < 0)
		edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
	// For disctinction between this case and the above one,
	// A hypothesis has a discontinuous hyper edge.
	else
		edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_STR);

	lm::ngram::Model::State out;
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
			left->GetTrgLeft(),
			right->GetTrgRight(),
			l+1 == r ? out : right->GetState(),
			0, 0, left_stack, right_stack));
	// Add the inverted terminal
	if (m < 0 && n < 0)
		edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
	else
		edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV);

	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
			right->GetTrgLeft(),
			left->GetTrgRight(),
			l+1 == r ? out : left->GetState(),
			0, 0, left_stack, right_stack));
}


void DiscontinuousHyperGraph::AddDiscontinuousHypotheses(
		ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent,	HypothesisQueue & q,
		int left_l, int left_m, int left_n, int left_r,
		int right_l, int right_m, int right_n, int right_r) {
	TargetSpan *left_stack, *right_stack;
	double score, viterbi_score, non_local_score;
	// Find the best hypotheses on the left and right side
	left_stack = GetStack(left_l, left_m, left_n, left_r);
	right_stack = GetStack(right_l, right_m, right_n, right_r);
	if (!left_stack)
		THROW_ERROR("SetStack first [" << left_l << ", " << left_m << ", " << left_n << ", " << left_r)
	if (!right_stack)
		THROW_ERROR("SetStack first [" << right_l << ", " << right_m << ", " << right_n << ", " << right_r)
	int l, m, n, r, c;
	HyperEdge * edge;
    Hypothesis * left, *right;
   	left = left_stack->GetHypothesis(0);
   	right = right_stack->GetHypothesis(0);
	// continuous + continuous = discontinuous
    if (left_m < 0 && left_m < 0 && right_m < 0 && right_n < 0){
        l = left_l; m = left_r; n = right_l; r = right_r; c = -1;
    }
    // continuous + discontinuous = discontinuous
    else if (left_m < 0 && left_n < 0 && left_r+1 == right_l){
        l = left_l; m = right_m; n = right_n; r = right_r; c = right_l;
    }
    // discontinuous + continuous = discontinuous
    else if (right_m < 0 && right_n < 0 && left_r+1 == right_l){
        l = left_l; m = left_m; n = left_n; r = right_r; c = right_l;
    }
	if (l > m || m+1 > n-1 || n > r)
		THROW_ERROR("Invalid Target Span "
				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]");
//    cerr << "Add discontinous str & inv "<<
//				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	lm::ngram::Model::State out;
	// Add the straight terminal
	edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_STR);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, non_local_score, edge,
			left->GetTrgLeft(), right->GetTrgRight(),
			l+1 == r ? out : right->GetState(),
			0, 0, left_stack, right_stack));
	// Add the inverted terminal
	edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, non_local_score, edge,
			right->GetTrgLeft(), left->GetTrgRight(),
			l+1 == r ? out : left->GetState(),
			0, 0, left_stack, right_stack));
}

// For cube pruning
void DiscontinuousHyperGraph::StartBeamSearch(
		int beam_size,
		HypothesisQueue & q, ReordererModel & model, const Sentence & sent,
		const FeatureSet & features, const FeatureSet & non_local_features,
		TargetSpan * ret, int l, int r)
{
    //cerr << "Start beam search among " << q.size() << " hypotheses" << endl;
    int num_processed = 0;
    int pop_count = 0;
    while((!beam_size || num_processed < beam_size) && q.size()){
        // Pop a hypothesis from the stack and get its target span
        Hypothesis *hyp = q.top(); q.pop();
        // skip unnecessary hypothesis
        // Insert the hypothesis if unique
        bool skip = CanSkip(hyp) || !ret->AddUniqueHypothesis(hyp);
        if (!skip){
        	num_processed++;
			if(verbose_ > 1){
				boost::mutex::scoped_lock lock(mutex_);
				DiscontinuousHypothesis * dhyp =
						dynamic_cast<DiscontinuousHypothesis*>(hyp);
				cerr << "Insert " << num_processed << "th hypothesis ";
				if (dhyp) cerr << *dhyp;
				else cerr << *hyp;
				cerr << endl;

				const FeatureVectorInt *fvi = GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
				FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
				FeatureVectorString *fws = model.StringifyWeightVector(*fvi);
				cerr << *fvs << endl << *fws;
				hyp->PrintChildren(cerr);
				// otherwise, do not delete fvi for edge, will be deleted later
				delete fvs, fws;
				// if fvi is not stored, delete
				if (!save_features_)
					delete fvi;
				Hypothesis * left =hyp->GetLeftHyp();
				Hypothesis * right =hyp->GetRightHyp();
				// termininals may have bigram features
				if((left && right) || hyp->IsTerminal()){
					lm::ngram::State out;
					const FeatureVectorInt * fvi = GetNonLocalFeatures(hyp->GetEdge(), left, right,
							non_local_features, sent, model, &out);
					if (fvi && fvi->size()){
						fvs = model.StringifyFeatureVector(*fvi);
						fws = model.StringifyWeightVector(*fvi);
						cerr << "/********************* non-local features ***********************/" << endl;
						cerr << *fvs << endl << *fws;
						delete fvi, fvs, fws;
					}
				}
				cerr << endl << "/****************************************************************/" << endl;
			}
        }

        // Add next cube items
    	Hypothesis * new_left = NULL, *new_right = NULL;

    	TargetSpan *left_span = hyp->GetLeftChild();
    	if (left_span)
    		new_left = left_span->GetHypothesis(hyp->GetLeftRank()+1);
    	IncrementLeft(hyp, new_left, model, non_local_features, sent, q, pop_count);

    	TargetSpan *right_span = hyp->GetRightChild();
    	if (right_span)
    		new_right = right_span->GetHypothesis(hyp->GetRightRank()+1);
    	IncrementRight(hyp, new_right, model, non_local_features, sent, q, pop_count);

    	if (skip)
    		delete hyp;
    }

    while(!q.empty()){
        delete q.top();
        q.pop();
    }
    sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
}

// Build a hypergraph using beam search and cube pruning
void DiscontinuousHyperGraph::ProcessOneDiscontinuousSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const FeatureSet & non_local_features,
		const Sentence & sent,
		int l, int m, int n, int r,
		int beam_size){
    // Get a map to store identical target spans
	TargetSpan * stack = GetStack(l, m, n, r);// new DiscontinuousTargetSpan(l, m, n, r);
	if (!stack)
		THROW_ERROR("SetStack is required: [" << l << ", " << m << ", " << n << ", " << r << "]")
	// This is required when -save_features=true
	stack->ClearHypotheses();
	HypothesisQueue * q;
	if (cube_growing_)
		q = &stack->GetCands();
    // Create the temporary data members for this span
	else
		q = new HypothesisQueue;

	double score, viterbi_score;
	if (verbose_ > 1){
		boost::mutex::scoped_lock lock(mutex_);
		cerr << "Process ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	}
	// continuous + continuous = discontinuous
	//cerr << "continuous + continuous = discontinuous" << endl;
	AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
			l, -1, -1, m,
			n, -1, -1, r);
	if (full_fledged_){
		// continuous + discontinuous = discontinuous
		//cerr << "continuous + discontinuous = discontinuous" << endl;
		for (int c = l+1 ; c <= m ; c++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
					l, -1, -1, c-1,
					c, m, n, r);
		// discontinuous + continuous = discontinuous
		//cerr << "discontinuous + continuous = discontinuous" << endl;
		for (int c = n+1 ; c <= r ; c++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
					l, m, n, c-1,
					c, -1, -1, r);
	}

	if (verbose_ > 1){
		boost::mutex::scoped_lock lock(mutex_);
		cerr << "Ready for search in ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]: " << q->size() << " hypotheses" << endl;
	}
    // For cube growing, search is lazy
    if (cube_growing_)
    	return;

    StartBeamSearch(beam_size, *q, model, sent, features, non_local_features, stack, l, r);
    delete q;
}

// Build a discontinuous hypergraph using beam search and cube pruning
// Override HyperGraph::ProcessOneSpan
void DiscontinuousHyperGraph::ProcessOneSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const FeatureSet & non_local_features,
		const Sentence & sent,
		int l, int r,
		int beam_size) {
    // Get a map to store identical target spans
    TargetSpan * stack = HyperGraph::GetStack(l, r);// new TargetSpan(l, r);
    if (!stack)
    	THROW_ERROR("SetStack is required: [" << l << ", " << r << "]")
    // This is required when -save_features=true
    stack->ClearHypotheses();
    HypothesisQueue * q;
	if (cube_growing_)
		q = &stack->GetCands();
    // Create the temporary data members for this span
	else
		q = new HypothesisQueue;
	double score;
	// If the length is OK, add a terminal
	AddTerminals(l, r, features, model, sent, q);

    bool hasPunct = false;
    for (int i = l; i <= r; i++ )
    	hasPunct |= sent[0]->isPunct(i);

	int N = n_ = sent[0]->GetNumWords();
	int D = gap_size_;
	if (verbose_ > 1){
		boost::mutex::scoped_lock lock(mutex_);
		cerr << "Process ["<<l<<", "<<r<<"]" << endl;
	}
	lm::ngram::Model::State out;
	for (int m = l+1 ; m <= r ; m++){
		if (hasPunct && mp_){ // monotone at punctuation
			TargetSpan *left_stack, *right_stack;
			left_stack = HyperGraph::GetStack(l, m-1);
			right_stack = HyperGraph::GetStack(m, r);
			if(left_stack == NULL) THROW_ERROR("Target l="<<l<<", c-1="<<m-1);
			if(right_stack == NULL) THROW_ERROR("Target c="<<m<<", r="<<r);
	        Hypothesis * left, *right;
        	left = left_stack->GetHypothesis(0);
        	right = right_stack->GetHypothesis(0);
			// Add the straight non-terminal
			HyperEdge * edge = new HyperEdge(l, m, r, HyperEdge::EDGE_STR);
			score = GetEdgeScore(model, features, sent, *edge);
			double 	non_local_score = GetNonLocalScore(model, non_local_features, sent,
					edge, left, right, &out);
			double viterbi_score = score + non_local_score +
					left->GetScore() + right->GetScore();
			q->push(new Hypothesis(viterbi_score, score, non_local_score, edge,
					left->GetTrgLeft(),
					right->GetTrgRight(),
					l+1 == r ? out : right->GetState(),
					0, 0, left_stack, right_stack));
			continue;
		}
		// continuous + continuous = continuous
		//cerr << "continuous + continuous = continuous" << endl;
		AddHypotheses(model, features, non_local_features, sent, *q,
				l, -1, -1, m-1,
				m, -1, -1, r);
		// discontinuous + discontinuous = continuous
		//cerr << "discontinuous + discontinuous = continuous" << endl;
		for (int c = m+1; c-m <= D; c++)
			for(int n = c+1; n-c <= D && n <= r;n++)
				AddHypotheses(model, features, non_local_features, sent, *q,
										l, m-1, c, n-1,
										m, c-1, n, r);
		// x + x = discontinuous
		for (int d = 1 ; d <= D ; d++)
			if ( IsXXD(l, m-1, m+d, r+d, D, N) )
				ProcessOneDiscontinuousSpan(
						model, features, non_local_features, sent,
						l, m-1, m+d, r+d,
						beam_size);
	}

	if (verbose_ > 1){
		boost::mutex::scoped_lock lock(mutex_);
		cerr << "Ready for search in ["<<l<<", "<<r<<"]: " << q->size() << " hypotheses" << endl;
	}
    // For cube growing, search is lazy
    if (cube_growing_)
    	return;

    StartBeamSearch(beam_size, *q, model, sent, features, non_local_features, stack, l, r);
    delete q;
}

// for cube growing, trigger the best hypothesis
void DiscontinuousHyperGraph::TriggerTheBestHypotheses(int l, int r,
		ReordererModel & model, const FeatureSet & non_local_features,
		const Sentence & sent) {
	int N = sent[0]->GetNumWords();
	int D = gap_size_;
	if (verbose_ > 1)
		cerr << "Trigger the best hypothesis " << *HyperGraph::GetStack(l, r) << endl;
	int pop_count = 0;
	Hypothesis * best = LazyKthBest(HyperGraph::GetStack(l, r), 0,
			model, non_local_features, sent, pop_count);
	if (!best)
		THROW_ERROR("Fail to produce hypotheses " << *HyperGraph::GetStack(l, r) << endl)
	for (int c = l+1 ; c <= r ; c++)
		for (int d = 1 ; d <= D ; d++)
			if ( IsXXD(l, c-1, c+d, r+d, D, N) ){
				if (verbose_ > 1)
					cerr << "Trigger the best hypothesis "
					<< *(DiscontinuousTargetSpan*)GetStack(l, c-1, c+d, r+d) << endl;
				pop_count = 0;
				best = LazyKthBest(GetStack(l, c-1, c+d, r+d), 0,
						model, non_local_features, sent, pop_count);
				if (!best)
					THROW_ERROR("Fail to produce hypotheses "
							<< *(DiscontinuousTargetSpan*)GetStack(l, c-1, c+d, r+d) << endl)
			}
}

// Build a hypergraph using beam search and cube pruning
void DiscontinuousHyperGraph::BuildHyperGraph(ReordererModel & model,
        const FeatureSet & features,
        const FeatureSet & non_local_features,
        const Sentence & sent){
	SetNumWords(sent[0]->GetNumWords());
	next_.resize(n_ * (n_+1) / 2 + 1, NULL); // resize next hyper-graph in advance
	if (verbose_ > 1)
		cerr << "BuildHyperGraph with " << threads_ << " threads for the same-sized stack" << endl;
	HyperGraph::BuildHyperGraph(model, features, non_local_features, sent);
}

// skip unnecessary hypothesis
bool DiscontinuousHyperGraph::CanSkip(Hypothesis * hyp)
{
    DiscontinuousHypothesis *dhyp = dynamic_cast<DiscontinuousHypothesis*>(hyp);
    Hypothesis *left = hyp->GetLeftHyp();
    Hypothesis *right = hyp->GetRightHyp();
    bool skip;
    if(dhyp)
        skip = DiscontinuousHypothesis::CanSkip(hyp->GetEdge(), left, right);

    else
        skip = Hypothesis::CanSkip(hyp->GetEdge(), left, right);

    return skip;
}
// For cube growing
Hypothesis * DiscontinuousHyperGraph::LazyKthBest(TargetSpan * stack, int k,
		ReordererModel & model, const FeatureSet & non_local_features,
		const Sentence & sent, int & pop_count) {
	while (((!beam_size_ && stack->HypSize() < k + 1)
			|| stack->HypSize() < std::min(k + 1, beam_size_))
			&& stack->CandSize() > 0) {
		HypothesisQueue & q = stack->GetCands();
		Hypothesis * hyp = q.top(); q.pop();
	    if (hyp->GetTrgLeft() < hyp->GetLeft()
		|| hyp->GetTrgRight() < hyp->GetLeft()
		|| hyp->GetRight() < hyp->GetTrgLeft()
		|| hyp->GetRight() < hyp->GetTrgRight()) {
			hyp->PrintParse(cerr);
			DiscontinuousHypothesis * dhyp =
					dynamic_cast<DiscontinuousHypothesis*>(hyp);
			THROW_ERROR("Malformed hypothesis in LazyKthBest " << (dhyp? *dhyp : *hyp) << endl)
		}
		// skip unnecessary hypothesis
	    // do not skip the first hypothesis
	    // it would be unnecessary, but ensures at least one hypothesis in stack
		// Insert the hypothesis if unique
	    bool skip = (k > 0 && CanSkip(hyp)) || !stack->AddUniqueHypothesis(hyp);
	    LazyNext(q, model, non_local_features, sent, hyp, pop_count);
		if (skip){
			if(verbose_ > 2){
				DiscontinuousHypothesis * dhyp =
						dynamic_cast<DiscontinuousHypothesis*>(hyp);
				cerr << "Prune ";
				if (dhyp) cerr << *dhyp;
				else cerr << *hyp;
				hyp->PrintChildren(cerr);
			}
			delete hyp;
		}
		else if(verbose_ > 2){
			DiscontinuousHypothesis * dhyp =
					dynamic_cast<DiscontinuousHypothesis*>(hyp);
			cerr << "Insert " << k << "th hypothesis ";
			if (dhyp) cerr << *dhyp;
			else cerr << *hyp;
			hyp->PrintChildren(cerr);
		}
		if (pop_limit_ && ++pop_count >= pop_limit_)
			break;
	}
	return stack->GetHypothesis(k);
}

// Accumulate edge features under a hyper-edge
// Accumulate non-local features under a hypothesis
void DiscontinuousHyperGraph::AccumulateFeatures(std::tr1::unordered_map<int,double> & feat_map,
										ReordererModel & model,
		                                const FeatureSet & features,
		                                const FeatureSet & non_local_features,
		                                const Sentence & sent,
		                                const Hypothesis * hyp){
	if (verbose_ > 1){
		const DiscontinuousHypothesis * dhyp =
				dynamic_cast<const DiscontinuousHypothesis*>(hyp);
		cerr << "Accumulate feature of hypothesis ";
		if (dhyp) cerr << *dhyp;
		else cerr << *hyp;
		cerr << endl;
	}
	Hypothesis * left = hyp->GetLeftHyp();
	Hypothesis * right = hyp->GetRightHyp();
	// root has no edge feature
	// root has only the bigram feature on the boundaries
	if (hyp->GetEdgeType() == HyperEdge::EDGE_ROOT){
		if (bigram_){
			lm::ngram::State out;
			double rootBigram = GetRootBigram(sent, hyp, &out);
			feat_map[model.GetFeatureIds().GetId("BIGRAM")] += rootBigram;
			if (verbose_ > 1){
	            cerr << "/********************* non-local features ***********************/" << endl;
				cerr << "BIGRAM:" << rootBigram << endl << "BIGRAM:" << model.GetWeight("BIGRAM") << endl;
				cerr << "/****************************************************************/" << endl;
			}
		}
	}
	else{
		// accumulate edge features
		const FeatureVectorInt * fvi = GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
		BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
			feat_map[feat_pair.first] += feat_pair.second;
		if (verbose_ > 1){
			cerr << "/****************************************************************/" << endl;
			FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
			FeatureVectorString *fws = model.StringifyWeightVector(*fvi);
			cerr << *fvs << endl << *fws << endl;
			// do not delete fvi for edge, will be deleted later
			delete fvs, fws;
		}
		// if fvi is not stored, delete
		if (!save_features_)
			delete fvi;
		// accumulate non-local features
		lm::ngram::State out;
		fvi = GetNonLocalFeatures(hyp->GetEdge(), left, right,
				non_local_features, sent, model, &out);
		if (fvi){
			if (verbose_ > 1 && fvi->size()){
				FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
				FeatureVectorString * fws = model.StringifyWeightVector(*fvi);
				cerr << "/********************* non-local features ***********************/" << endl;
				cerr << *fvs << endl << *fws << endl;
				delete fvs, fws;
			}
			BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
            		feat_map[feat_pair.first] += feat_pair.second;
			delete fvi;
		}
		if (verbose_ > 1)
			cerr << "/****************************************************************/" << endl;
	}
	if (left)
		AccumulateFeatures(feat_map, model, features, non_local_features, sent, left);
	if (right)
		AccumulateFeatures(feat_map, model, features, non_local_features, sent, right);
}

void DiscontinuousHyperGraph::AddLoss(LossBase* loss,
		const Ranks * ranks, const FeatureDataParse * parse) const{
    // Initialize the loss
    loss->Initialize(ranks, parse);
    // For each span in the hypergraph
    int N = n_;
    int D = gap_size_;
    if (verbose_ > 1){
    	cerr << "Rank:";
    	BOOST_FOREACH(int rank, ranks->GetRanks())
    		cerr << " " << rank;
    	cerr << endl;
    }
    for(int r = 0; r <= N; r++) {
        // When r == n, we want the root, so only do -1
        for(int l = (r == N ? -1 : 0); l <= (r == N ? -1 : r); l++) {
        	if (verbose_ > 1)
        		cerr << "AddLoss ["<<l<<", "<<r<<"]" << endl;
        	BOOST_FOREACH(Hypothesis* hyp, HyperGraph::GetStack(l,r)->GetHypotheses()) {
        	    if (hyp->GetTrgLeft() < hyp->GetLeft()
				|| hyp->GetTrgRight() < hyp->GetLeft()
				|| hyp->GetRight() < hyp->GetTrgLeft()
				|| hyp->GetRight() < hyp->GetTrgRight()) {
        	    	hyp->PrintParse(cerr);
        	    	DiscontinuousHypothesis * dhyp =
        	    			dynamic_cast<DiscontinuousHypothesis*>(hyp);
        	    	THROW_ERROR("Malformed hypothesis in AddLossToProduction " << (dhyp? *dhyp : *hyp) << endl)
        	    }
				hyp->SetLoss(hyp->GetLoss() +
							loss->AddLossToProduction(hyp, ranks, parse));
				if (verbose_ > 1){
					cerr << "Loss=" << hyp->GetLoss() << ":" << *hyp;
					if (hyp->GetCenter() == -1)
						cerr << endl;
					else
						hyp->PrintChildren(cerr);
				}
            }
            if (l < 0)
            	continue;
            for (int c = l+1 ; c <= r ; c++){
            	for (int d = 1 ; d <= D ; d++){
            		if ( IsXXD(l, c-1, c+d, r+d, D, N) ){
						if (verbose_ > 1)
							cerr << "AddLoss ["<<l<<", "<<c-1<<", "<<c+d<<", "<<r+d<<"]" << endl;
						BOOST_FOREACH(Hypothesis* hyp, GetStack(l,c-1,c+d,r+d)->GetHypotheses()) {
							hyp->SetLoss(hyp->GetLoss() +
									loss->AddLossToProduction(hyp, ranks, parse));
							if (verbose_ > 1){
								DiscontinuousHypothesis * dhyp =
										dynamic_cast<DiscontinuousHypothesis*>(hyp);
								cerr << "Loss=" << hyp->GetLoss() << ":" << *dhyp;
								hyp->PrintChildren(cerr);
							}
						}
					}
            	}
        	}
        }
    }
}
