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
#include <lader/discontinuous-target-span.h>
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

void DiscontinuousHyperGraph::AddHyperEdges(
		ReordererModel & model, const FeatureSet & features,
		const Sentence & sent,	HypothesisQueue & q,
		int left_l, int left_m, int left_n, int left_r,
		int right_l, int right_m, int right_n, int right_r)
{
	TargetSpan *left_trg, *right_trg;
	double score, viterbi_score;
	// Find the best hypotheses on the left and right side
	left_trg = GetTrgSpan(left_l, left_m, left_n, left_r, 0);
	right_trg = GetTrgSpan(right_l, right_m, right_n, right_r, 0);
	if(left_trg == NULL) THROW_ERROR("Null left_trg "
			"["<<left_l<<", "<<left_m<<", "<<left_n<<", "<<left_r<<"]");
	if(right_trg == NULL) THROW_ERROR("Null right_trg "
			"["<<right_l<<", "<<right_m<<", "<<right_n<<", "<<right_r<<"]");
	int l, c, r;
	// continuous + continuous = continuous
	if (left_m < 0 && left_n < 0 && right_m < 0 && right_n < 0 && left_r+1 == right_l){
		l = left_l;
		c = right_l;
		r = right_r;
	}
//	// discontinuous + continuous = continuous
//	else if (right_m < 0 && right_n < 0 && left_m+1 == right_l && right_r+1 == left_n){
//		l = left_l;
//		c = right_l; // TODO: distinguish this case
//		r = left_r;
//	}
	// discontinuous + discontinuous = continuous
	else if (left_m+1 == right_l && right_m+1 == left_n && left_r+1 == right_n){
		l = left_l;
		c = left_n; // TODO: distinguish this case
		r = right_r;
	}
	if (l >= c || c > r)
		THROW_ERROR("Invalid Target Span ["<<l<<", "<<c<<", "<<r<<"]");
	// Add the straight terminal
	HyperEdge * edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
	score = GetEdgeScore(model, features, sent, *edge);
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(new Hypothesis(viterbi_score, score, edge,
			left_trg->GetTrgLeft(), right_trg->GetTrgRight(),
			0, 0, left_trg, right_trg));
	// Add the inverted terminal
	edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(new Hypothesis(viterbi_score, score, edge,
			right_trg->GetTrgLeft(), left_trg->GetTrgRight(),
			0, 0, left_trg, right_trg));
}


void DiscontinuousHyperGraph::AddDiscontinuousHyperEdges(
		ReordererModel & model, const FeatureSet & features,
		const Sentence & sent,	HypothesisQueue & q,
		int left_l, int left_m, int left_n, int left_r,
		int right_l, int right_m, int right_n, int right_r) {
	TargetSpan *left_trg, *right_trg;
	double score, viterbi_score;
	// Find the best hypotheses on the left and right side
	left_trg = GetTrgSpan(left_l, left_m, left_n, left_r, 0);
	right_trg = GetTrgSpan(right_l, right_m, right_n, right_r, 0);
	if(left_trg == NULL) THROW_ERROR("Null left_trg "
			"["<<left_l<<", "<<left_m<<", "<<left_n<<", "<<left_r<<"]");
	if(right_trg == NULL) THROW_ERROR("Null right_trg "
			"["<<right_l<<", "<<right_m<<", "<<right_n<<", "<<right_r<<"]");
	int l, m, n, r;
    if (left_m < 0 && left_m < 0 && right_m < 0 && right_n < 0){
        l = left_l; m = left_r; n = right_l; r = right_r;
    }
//    else if (left_m < 0 && left_n < 0 && left_r+1 == right_l){
//        l = left_l; m = right_m; n = right_n; r = right_r;
//    }
//    else if (right_m < 0 && right_n < 0 && left_r+1 == right_l){
//        l = left_l; m = left_m; n = left_n; r = right_r;
//    }
    else if (right_m < 0 && right_n < 0 && left_m+1 == right_l){
        l = left_l; m = right_r; n = left_n; r = left_r;
    }
    else if (right_m < 0 && right_n < 0 && right_r+1 == left_n){
        l = left_l; m = left_m; n = right_l; r = left_r;
    }
	if (l > m || m+1 > n-1 || n > r)
		THROW_ERROR("Invalid Target Span "
				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]");
//    cerr << "Add discontinous str & inv "<<
//				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	// Add the straight terminal
	HyperEdge * edge = new DiscontinuousHyperEdge(l, m, n, r, HyperEdge::EDGE_STR);
	score = GetEdgeScore(model, features, sent, *edge);
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, edge,
			left_trg->GetTrgLeft(), right_trg->GetTrgRight(),
			0, 0, left_trg, right_trg));
	// Add the inverted terminal
	edge = new DiscontinuousHyperEdge(l, m, n, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, edge,
			right_trg->GetTrgLeft(), left_trg->GetTrgRight(),
			0, 0, left_trg, right_trg));
}

void DiscontinuousHyperGraph::nextCubeItems(const Hypothesis * hyp,
		HypothesisQueue & q, int l, int r, bool gap)
{
	TargetSpan
	*new_left_trg = NULL, *old_left_trg = NULL,
	*new_right_trg = NULL, *old_right_trg = NULL;
	DiscontinuousTargetSpan *child;
	// Increment the left side if there is still a hypothesis left
	child = dynamic_cast<DiscontinuousTargetSpan*>(hyp->GetLeftChild());
	if (child != NULL){
		new_left_trg = GetTrgSpan(
				child->GetLeft(), child->GetM(),
				child->GetN(), child->GetRight(), hyp->GetLeftRank()+1);
		if (new_left_trg)
			old_left_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp->GetLeftRank());
	}
	else if (hyp->GetCenter() != -1){
		new_left_trg = HyperGraph::GetTrgSpan(l, hyp->GetCenter()-1, hyp->GetLeftRank()+1);
		if(new_left_trg)
			old_left_trg = HyperGraph::GetTrgSpan(l,hyp->GetCenter()-1,hyp->GetLeftRank());
	}
	Hypothesis * new_hyp;
	if (new_left_trg != NULL && old_left_trg != NULL){
		if (gap){
			DiscontinuousHyperEdge * edge =
					dynamic_cast<DiscontinuousHyperEdge*>(hyp->GetEdge());
			new_hyp = new DiscontinuousHypothesis(*hyp);
			new_hyp->SetEdge(new DiscontinuousHyperEdge(*edge));
		}
		else{
			new_hyp = new Hypothesis(*hyp);
			new_hyp->SetEdge(new HyperEdge(*hyp->GetEdge()));
		}
		new_hyp->SetScore(hyp->GetScore() - old_left_trg->GetScore() + new_left_trg->GetScore());
		new_hyp->SetLeftRank(hyp->GetLeftRank() + 1);
		new_hyp->SetLeftChild(new_left_trg);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR){
			new_hyp->SetTrgLeft(new_left_trg->GetTrgLeft());
		}else{
			new_hyp->SetTrgRight(new_left_trg->GetTrgRight());
		}
		q.push(new_hyp);
	}
	// Increment the right side if there is still a hypothesis right
	child = dynamic_cast<DiscontinuousTargetSpan*>(hyp->GetRightChild());
	if (child != NULL){
		new_right_trg = GetTrgSpan(
				child->GetLeft(), child->GetM(),
				child->GetN(), child->GetRight(), hyp->GetRightRank()+1);
		if(new_right_trg)
			old_right_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp->GetRightRank());
	}
	else if (hyp->GetCenter() != -1){
		new_right_trg = HyperGraph::GetTrgSpan(hyp->GetCenter(), r, hyp->GetRightRank()+1);
		if(new_right_trg)
			old_right_trg = HyperGraph::GetTrgSpan(hyp->GetCenter(), r, hyp->GetRightRank());
	}

	if (new_right_trg != NULL && old_right_trg != NULL){
		if (gap){
			DiscontinuousHyperEdge * edge =
					dynamic_cast<DiscontinuousHyperEdge*>(hyp->GetEdge());
			new_hyp = new DiscontinuousHypothesis(*hyp);
			new_hyp->SetEdge(new DiscontinuousHyperEdge(*edge));
		}
		else{
			new_hyp = new Hypothesis(*hyp);
			new_hyp->SetEdge(new HyperEdge(*hyp->GetEdge()));
		}
		new_hyp->SetScore(hyp->GetScore()
				- old_right_trg->GetScore() + new_right_trg->GetScore());
		new_hyp->SetRightRank(hyp->GetRightRank()+1);
		new_hyp->SetRightChild(new_right_trg);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR) {
			new_hyp->SetTrgRight(new_right_trg->GetTrgRight());
		} else {
			new_hyp->SetTrgLeft(new_right_trg->GetTrgLeft());
		}
		q.push(new_hyp);
	}
}

// Build a hypergraph using beam search and cube pruning
SpanStack *DiscontinuousHyperGraph::ProcessOneDiscontinuousSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const Sentence & sent,
		int l, int m, int n, int r,
		int beam_size, bool save_trg){
	HypothesisQueue q;
	double score, viterbi_score;
	if (verbose_)
		cerr << "AddDiscontinuousHyperEdges ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	// continuous + continuous = discontinuous
	//cerr << "continuous + continuous = discontinuous" << endl;
	AddDiscontinuousHyperEdges(model, features, sent, q,
			l, -1, -1, m,
			n, -1, -1, r);
//	// continuous + discontinuous = discontinuous
//	//cerr << "continuous + discontinuous = discontinuous" << endl;
//	for (int i = l+1 ; i <= m ; i++)
//		AddDiscontinuousHyperEdges(model, features, sent, q,
//				l, -1, -1, i-1,
//				i, m, n, r);
//	// discontinuous + continuous = discontinuous
//	//cerr << "discontinuous + continuous = discontinuous" << endl;
//	for (int i = n+1 ; i <= r ; i++)
//		AddDiscontinuousHyperEdges(model, features, sent, q,
//				l, m, n, i-1,
//				i, -1, -1, r);
	// discontinuous + continuous = discontinuous
	//cerr << "discontinuous + left continuous = discontinuous" << endl;
	for (int i = m ; i > l && n-i <= gap_; i--)
		AddDiscontinuousHyperEdges(model, features, sent, q,
				l, i-1, n, r,
				i, -1, -1, m);
	// discontinuous + continuous = discontinuous
	//cerr << "discontinuous + right continuous = discontinuous" << endl;
	for (int i = r ; i > n && i-m-1 <= gap_ ; i--)
		AddDiscontinuousHyperEdges(model, features, sent, q,
				l, m, i, r,
				n, -1, -1, i-1);

	TargetSpan
		*new_left_trg, *old_left_trg,
		*new_right_trg, *old_right_trg;
	// Get a map to store identical target spans
	tr1::unordered_map<int, TargetSpan*> spans;
	int r_max = r+1;
	TargetSpan * trg_span = NULL;
	// Start beam search
	//cerr << "Start beam search among " << q.size() << " hypotheses" << endl;
	int num_processed = 0;
	while((!beam_size || num_processed < beam_size) && q.size()) {
		// Pop a hypothesis from the stack and get its target span
		DiscontinuousHypothesis * hyp =
				dynamic_cast<DiscontinuousHypothesis*>(q.top()); q.pop();
		int trg_idx = hyp->GetTrgLeft()*r_max+hyp->GetTrgRight();
		tr1::unordered_map<int, TargetSpan*>::iterator it = spans.find(trg_idx);
		if(it != spans.end()) {
			trg_span = it->second;
		} else{
			trg_span = new DiscontinuousTargetSpan(
					hyp->GetLeft(), hyp->GetM(), hyp->GetN(), hyp->GetRight(),
					hyp->GetTrgLeft(), hyp->GetTrgRight());
			spans.insert(MakePair(trg_idx, trg_span));
		}
		// Insert the hypothesis
		if (verbose_){
			cerr << "Insert the discontinuous hypothesis " << *hyp << endl;
			const FeatureVectorInt * fvi = HyperGraph::GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			cerr << *fvs << endl;
			delete fvs;
		}
		trg_span->AddHypothesis(*hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && q.top() == hyp) {
			delete q.top();
			q.pop();
		}
		// Skip terminals
//		if(hyp->GetCenter() == -1) continue;
		// Increment the left side if there is still a hypothesis left
		nextCubeItems(hyp, q, l, r, true);
		delete hyp;
	}
	while(q.size()) {
		delete q.top();
		q.pop();
	}
    //cerr << "sort discontinuous spans obtained by cube pruning: size " << spans.size() << endl;
	SpanStack * ret = new SpanStack;
	typedef pair<int, TargetSpan*> MapPair;
	BOOST_FOREACH(const MapPair & map_pair, spans)
		ret->AddSpan(map_pair.second);
	sort(ret->GetSpans().begin(), ret->GetSpans().end(), DescendingScore<TargetSpan>());
    //cerr << "return sorted spans: size " << ret->size() << endl;
	return ret;
}

// Build a discontinuous hypergraph using beam search and cube pruning
// Override HyperGraph::ProcessOneSpan
SpanStack * DiscontinuousHyperGraph::ProcessOneSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const Sentence & sent,
		int l, int r,
		int beam_size, bool save_trg) {
	// Create the temporary data members for this span
	HypothesisQueue q;
	double score;
	// If the length is OK, add a terminal
	if((features.GetMaxTerm() == 0) || (r-l < features.GetMaxTerm())) {
		int tl = (save_trg ? l : -1);
		int tr = (save_trg ? r : -1);
		// Create a hypothesis with the forward terminal
		HyperEdge * edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_FOR);
		score = GetEdgeScore(model, features, sent, *edge);
		q.push(new Hypothesis(score, score, edge, tl, tr));
		if(features.GetUseReverse()) {
			// Create a hypothesis with the backward terminal
			edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_BAC);
			score = GetEdgeScore(model, features, sent, *edge);
			q.push(new Hypothesis(score, score, edge, tr, tl));
		}
	}



    bool hasPunct = false;
    for (int i = l; i <= r; i++ )
    	hasPunct |= sent[0]->isPunct(i);

	int N = n_ = sent[0]->GetNumWords();
	int D = gap_;
	if (verbose_)
		cerr << "AddHyperEdges ["<<l<<", "<<r<<"]" << endl;
	for (int i = l+1 ; i <= r ; i++){
		if (hasPunct && mp_){ // monotone at punctuation
			TargetSpan *left_trg, *right_trg;
			left_trg = HyperGraph::GetTrgSpan(l, i-1, 0);
			right_trg = HyperGraph::GetTrgSpan(i, r, 0);
			if(left_trg == NULL) THROW_ERROR("Target l="<<l<<", c-1="<<i-1);
			if(right_trg == NULL) THROW_ERROR("Target c="<<i<<", r="<<r);
			// Add the straight non-terminal
			HyperEdge * edge = new HyperEdge(l, i, r, HyperEdge::EDGE_STR);
			score = GetEdgeScore(model, features, sent, *edge);
			double viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
			q.push(new Hypothesis(viterbi_score, score, edge,
					left_trg->GetTrgLeft(), right_trg->GetTrgRight(),
					0, 0, left_trg, right_trg));
			continue;
		}
		// continuous + continuous = continuous
		//cerr << "continuous + continuous = continuous" << endl;
		AddHyperEdges(model, features, sent, q,
				l, -1, -1, i-1,
				i, -1, -1, r);
		for (int d = 1 ; d <= D ; d++){
//			// discontinuous + continuous = continuous
//			//cerr << "discontinuous + continuous = continuous" << endl;
//			if ( i+d <= r ){
//				AddHyperEdges(model, features, sent, q,
//						l, i-1, i+d, r,
//						i, -1, -1, i-1+d);
//			}
			// discontinuous + discontinuous = continuous
			//cerr << "discontinuous + discontinuous = continuous" << endl;
			for (int j = i+d+1 ; j <= r && j - (i + d) <= D ; j++){
				AddHyperEdges(model, features, sent, q,
						l, i-1, i+d, j-1,
						i, i-1+d, j, r);
			}
			if ( r + d < N){
				SetStack(l, i-1, i+d, r+d,
						ProcessOneDiscontinuousSpan(
								model, features, sent,
								l, i-1, i+d, r+d,
								beam_size, save_trg));
			}
		}
	}
	// Get a map to store identical target spans
	tr1::unordered_map<int, TargetSpan*> spans;
	int r_max = r+1;
	TargetSpan * trg_span = NULL;
	// Start beam search
	int num_processed = 0;
	while((!beam_size || num_processed < beam_size) && q.size()) {
		// Pop a hypothesis from the stack and get its target span
		Hypothesis * hyp = q.top(); q.pop();
		int trg_idx = hyp->GetTrgLeft()*r_max+hyp->GetTrgRight();
		tr1::unordered_map<int, TargetSpan*>::iterator it = spans.find(trg_idx);
		if(it != spans.end()) {
			trg_span = it->second;
		} else {
			trg_span = new TargetSpan(hyp->GetLeft(), hyp->GetRight(),
					hyp->GetTrgLeft(), hyp->GetTrgRight());
			spans.insert(MakePair(trg_idx, trg_span));
		}
		// Insert the hypothesis
		if (verbose_){
			cerr << "Insert the hypothesis " << *hyp << endl;
			const FeatureVectorInt * fvi = HyperGraph::GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			cerr << *fvs << endl;
			delete fvs;
		}
		trg_span->AddHypothesis(*hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && q.top() == hyp) {
			delete q.top();
			q.pop();
		}
		// Skip terminals
//		if(hyp->GetCenter() == -1) continue;
		nextCubeItems(hyp, q, l, r, false);
		delete hyp;
	}
	while(q.size()) {
		delete q.top();
		q.pop();
	}
    //cerr << "sort continuous spans obtained by cube pruning: size " << spans.size() << endl;
	SpanStack * ret = new SpanStack;
	typedef pair<int, TargetSpan*> MapPair;
	BOOST_FOREACH(const MapPair & map_pair, spans)
		ret->AddSpan(map_pair.second);
	sort(ret->GetSpans().begin(), ret->GetSpans().end(), DescendingScore<TargetSpan>());
    //cerr << "return sorted spans: size " << ret->size() << endl;
	return ret;
}

void DiscontinuousHyperGraph::AddLoss(LossBase* loss,
		const Ranks * ranks, const FeatureDataParse * parse) const{
    // Initialize the loss
    loss->Initialize(ranks, parse);
    // For each span in the hypergraph
    int N = n_;
    int D = gap_;
    for(int r = 0; r <= N; r++) {
        // When r == n, we want the root, so only do -1
        for(int l = (r == N ? -1 : 0); l <= (r == N ? -1 : r); l++) {
            // DEBUG cerr << "l=" << l << ", r=" << r << ", n=" << n << endl;
            BOOST_FOREACH(TargetSpan* span, HyperGraph::GetStack(l,r)->GetSpans()) {
                BOOST_FOREACH(Hypothesis* hyp, span->GetHypotheses()) {
                    // DEBUG cerr << "GetLoss = " <<hyp->GetLoss()<<endl;
                    hyp->SetLoss(hyp->GetLoss() +
                    			loss->AddLossToProduction(hyp, ranks, parse));
                }
            }
            if (l < 0)
            	continue;
            for (int i = l+1 ; i <= r ; i++){
            	for (int d = 1 ; d <= D ; d++){
					if ( i+d <= r && r+d < N && GetStack(l,i-1,i+d,r) != NULL){
						// cerr << "AddLoss ["<<l<<", "<<i-1<<", "<<i+d<<", "<<r<<"]" << endl;
						BOOST_FOREACH(TargetSpan* span, GetStack(l,i-1,i+d,r)->GetSpans()) {
							BOOST_FOREACH(Hypothesis* hyp, span->GetHypotheses()) {
								// DEBUG cerr << "GetLoss = " <<hyp->GetLoss()<<endl;
								hyp->SetLoss(hyp->GetLoss() +
										loss->AddLossToProduction(hyp, ranks, parse));
							}
						}
					}
            	}
        	}
        }
    }
}

double DiscontinuousHyperGraph::Rescore(const ReordererModel & model, double loss_multiplier) {
	BOOST_FOREACH(HyperGraph * graph, next_)
		if (graph != NULL)
			// Reset everything to -DBL_MAX to indicate it needs to be recalculated
			BOOST_FOREACH(SpanStack * stack, graph->GetStacks())
				if (stack != NULL) // discontinuous span stack could be NULL
					BOOST_FOREACH(TargetSpan * trg, stack->GetSpans())
						BOOST_FOREACH(Hypothesis * hyp, trg->GetHypotheses())
			        		hyp->SetScore(-DBL_MAX);
	return HyperGraph::Rescore(model, loss_multiplier);
}
