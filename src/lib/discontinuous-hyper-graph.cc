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
	// discontinuous + continuous = continuous
	else if (right_m < 0 && right_n < 0 && left_m+1 == right_l && right_r+1 == left_n){
		l = left_l;
		c = right_l;
		r = left_r;
	}
		// discontinuous + discontinuous = continuous
	else if (left_m+1 == right_l && right_m+1 == left_n && left_r+1 == right_n){
		l = left_l;
		c = left_n;
		r = right_r;
	}
	if (l >= c || c > r)
		THROW_ERROR("Invalid Target Span ["<<l<<", "<<c<<", "<<r<<"]");
	// Add the straight terminal
	score = GetEdgeScore(model, features, sent,
			HyperEdge(l, c, r, HyperEdge::EDGE_STR));
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(Hypothesis(viterbi_score, score, l, r,
			left_trg->GetTrgLeft(), right_trg->GetTrgRight(),
			HyperEdge::EDGE_STR, c, 0, 0, left_trg, right_trg));
	// Add the inverted terminal
	score = GetEdgeScore(model, features, sent,
			HyperEdge(l, c, r, HyperEdge::EDGE_INV));
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(Hypothesis(viterbi_score, score, l, r,
			right_trg->GetTrgLeft(), left_trg->GetTrgRight(),
			HyperEdge::EDGE_INV, c, 0, 0, left_trg, right_trg));
}

void DiscontinuousHyperGraph::AddDiscontinuousHyperEdges(
		ReordererModel & model, const FeatureSet & features,
		const Sentence & sent,	DiscontinuousHypothesisQueue & q,
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
    else if (left_m < 0 && left_n < 0 && left_r+1 == right_l){
        l = left_l; m = right_m; n = right_n; r = right_r;
    }
    else if (right_m < 0 && right_n < 0 && left_r+1 == right_l){
        l = left_l; m = left_m; n = left_n; r = right_r;
    }
    else if (right_m < 0 && right_n < 0 && left_m+1 == right_l){
        l = left_l; m = right_r; n = left_n; r = left_r;
    }
    else if (right_m < 0 && right_n < 0 && right_r+1 == left_n){
        l = left_l; m = left_n; n = right_l; r = left_r;
    }
	if (l > m || m+1 > n-1 || n > r)
		THROW_ERROR("Invalid Target Span "
				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]");
//    //cerr << "Add discontinous str & inv "<<
//				"["<<l<<", "<<m<<", "<<n<<", "<<r<<"]";
	// Add the straight terminal
	score = GetEdgeScore(model, features, sent,
			DiscontinuousHyperEdge(l, m, n, r, HyperEdge::EDGE_STR));
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(DiscontinuousHypothesis(viterbi_score, score, l, m, n, r,
			left_trg->GetTrgLeft(), right_trg->GetTrgRight(),
			HyperEdge::EDGE_STR, -1, 0, 0, left_trg, right_trg));
	// Add the inverted terminal
	score = GetEdgeScore(model, features, sent,
			DiscontinuousHyperEdge(l, m, n, r, HyperEdge::EDGE_INV));
	viterbi_score = score + left_trg->GetScore() + right_trg->GetScore();
	q.push(DiscontinuousHypothesis(viterbi_score, score, l, m, n, r,
			right_trg->GetTrgLeft(), left_trg->GetTrgRight(),
			HyperEdge::EDGE_INV, -1, 0, 0, left_trg, right_trg));
}

// Build a hypergraph using beam search and cube pruning
SpanStack *DiscontinuousHyperGraph::ProcessOneDiscontinuousSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const Sentence & sent,
		int l, int m, int n, int r,
		int beam_size, bool save_trg){
	DiscontinuousHypothesisQueue q;
	double score, viterbi_score;
	int d = n - m;
	//cerr << "AddDiscontinuousHyperEdges ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	// continuous + continuous = discontinuous
	//cerr << "continuous + continuous = discontinuous" << endl;
	AddDiscontinuousHyperEdges(model, features, sent, q,
			l, -1, -1, m,
			n, -1, -1, r);
	// continuous + discontinuous = discontinuous
	//cerr << "continuous + discontinuous = discontinuous" << endl;
	for (int i = l+1 ; i <= m ; i++)
		AddDiscontinuousHyperEdges(model, features, sent, q,
				l, -1, -1, i-1,
				i, m, n, r);
	// discontinuous + continuous = discontinuous
	//cerr << "discontinuous + continuous = discontinuous" << endl;
	for (int i = n+1 ; i <= r ; i++)
		AddDiscontinuousHyperEdges(model, features, sent, q,
				l, m, n, i-1,
				i, -1, -1, r);
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
				i-1, -1, -1, n);

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
		Hypothesis top = q.top();
		// Pop a hypothesis from the stack and get its target span
		DiscontinuousHypothesis hyp = q.top(); q.pop();
		int trg_idx = hyp.GetTrgLeft()*r_max+hyp.GetTrgRight();
		tr1::unordered_map<int, TargetSpan*>::iterator it = spans.find(trg_idx);
		if(it != spans.end()) {
			trg_span = it->second;
		} else{
			trg_span = new DiscontinuousTargetSpan(
					hyp.GetLeft(), hyp.GetM(), hyp.GetN(), hyp.GetRight(),
					hyp.GetTrgLeft(), hyp.GetTrgRight());
			spans.insert(MakePair(trg_idx, trg_span));
		}
		// Insert the hypothesis
//		//cerr << "Insert the hypothesis "
//				"[" << hyp.GetLeft() << ", " << hyp.GetM() << ", " <<
//				hyp.GetN() << ", " << hyp.GetRight() << "]" << endl;
		trg_span->AddHypothesis(hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && q.top() == hyp) q.pop();
		// Skip terminals
//		if(hyp.GetCenter() == -1) continue;
		// Increment the left side if there is still a hypothesis left
		DiscontinuousTargetSpan * child;
		child = dynamic_cast<DiscontinuousTargetSpan*>(hyp.GetLeftChild());
		if (child != NULL){
			new_left_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp.GetLeftRank()+1);
			if (new_left_trg){
				old_left_trg = GetTrgSpan(
						child->GetLeft(), child->GetM(),
						child->GetN(), child->GetRight(), hyp.GetLeftRank());
				DiscontinuousHypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_left_trg->GetScore() + new_left_trg->GetScore());
				new_hyp.SetLeftRank(hyp.GetLeftRank()+1);
				new_hyp.SetLeftChild(new_left_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgLeft(new_left_trg->GetTrgLeft());
				} else {
					new_hyp.SetTrgRight(new_left_trg->GetTrgRight());
				}
				q.push(new_hyp);
			}
		}
		else if (hyp.GetCenter() != -1){
			new_left_trg = HyperGraph::GetTrgSpan(l, hyp.GetCenter()-1, hyp.GetLeftRank()+1);
			if(new_left_trg) {
				old_left_trg = HyperGraph::GetTrgSpan(l,hyp.GetCenter()-1,hyp.GetLeftRank());
				DiscontinuousHypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_left_trg->GetScore() + new_left_trg->GetScore());
				new_hyp.SetLeftRank(hyp.GetLeftRank()+1);
				new_hyp.SetLeftChild(new_left_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgLeft(new_left_trg->GetTrgLeft());
				} else {
					new_hyp.SetTrgRight(new_left_trg->GetTrgRight());
				}
				q.push(new_hyp);
			}
		}
		child = dynamic_cast<DiscontinuousTargetSpan*>(hyp.GetRightChild());
		if (child != NULL){
			new_right_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp.GetRightRank()+1);
			if(new_right_trg) {
				old_right_trg = GetTrgSpan(
						child->GetLeft(), child->GetM(),
						child->GetN(), child->GetRight(), hyp.GetRightRank());
				DiscontinuousHypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_right_trg->GetScore() + new_right_trg->GetScore());
				new_hyp.SetRightRank(hyp.GetRightRank()+1);
				new_hyp.SetRightChild(new_right_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgRight(new_right_trg->GetTrgRight());
				} else {
					new_hyp.SetTrgLeft(new_right_trg->GetTrgLeft());
				}
				q.push(new_hyp);
			}
		}
		else if (hyp.GetCenter() != -1){
			// Increment the right side if there is still a hypothesis right
			new_right_trg = HyperGraph::GetTrgSpan(hyp.GetCenter(), r, hyp.GetRightRank()+1);
			if(new_right_trg) {
				old_right_trg = HyperGraph::GetTrgSpan(hyp.GetCenter(), r, hyp.GetRightRank());
				DiscontinuousHypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_right_trg->GetScore() + new_right_trg->GetScore());
				new_hyp.SetRightRank(hyp.GetRightRank()+1);
				new_hyp.SetRightChild(new_right_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgRight(new_right_trg->GetTrgRight());
				} else {
					new_hyp.SetTrgLeft(new_right_trg->GetTrgLeft());
				}
				q.push(new_hyp);
			}
		}
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

// Build a hypergraph using beam search and cube pruning
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
		score = GetEdgeScore(model, features, sent,
				HyperEdge(l, -1, r, HyperEdge::EDGE_FOR));
		q.push(Hypothesis(score, score, l, r, tl, tr, HyperEdge::EDGE_FOR));
		if(features.GetUseReverse()) {
			// Create a hypothesis with the backward terminal
			score = GetEdgeScore(model, features, sent,
					HyperEdge(l, -1, r, HyperEdge::EDGE_BAC));
			q.push(Hypothesis(score, score, l, r, tr, tl, HyperEdge::EDGE_BAC));
		}
	}
	TargetSpan
	*new_left_trg, *old_left_trg,
	*new_right_trg, *old_right_trg;
	int N = n_ = sent[0]->GetNumWords();;
	int D = gap_;
	//cerr << "AddHyperEdges ["<<l<<", "<<r<<"]" << endl;
	for (int i = l+1 ; i <= r ; i++){
		// continuous + continuous = continuous
		//cerr << "continuous + continuous = continuous" << endl;
		AddHyperEdges(model, features, sent, q,
				l, -1, -1, i-1,
				i, -1, -1, r);
		for (int d = 1 ; d <= D ; d++){
			// discontinuous + continuous = continuous
			//cerr << "discontinuous + continuous = continuous" << endl;
			if ( i+d <= r ){
				AddHyperEdges(model, features, sent, q,
						l, i-1, i+d, r,
						i, -1, -1, i-1+d);
			}
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
		Hypothesis hyp = q.top(); q.pop();
		int trg_idx = hyp.GetTrgLeft()*r_max+hyp.GetTrgRight();
		tr1::unordered_map<int, TargetSpan*>::iterator it = spans.find(trg_idx);
		if(it != spans.end()) {
			trg_span = it->second;
		} else {
			trg_span = new TargetSpan(hyp.GetLeft(), hyp.GetRight(),
					hyp.GetTrgLeft(), hyp.GetTrgRight());
			spans.insert(MakePair(trg_idx, trg_span));
		}
		// Insert the hypothesis
		trg_span->AddHypothesis(hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && q.top() == hyp) q.pop();
		// Skip terminals
		if(hyp.GetCenter() == -1) continue;
		// Increment the left side if there is still a hypothesis left
		DiscontinuousTargetSpan * child;
		child = dynamic_cast<DiscontinuousTargetSpan*>(hyp.GetLeftChild());
		if (child != NULL){
			new_left_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp.GetLeftRank()+1);
			if (new_left_trg){
				old_left_trg = GetTrgSpan(
						child->GetLeft(), child->GetM(),
						child->GetN(), child->GetRight(), hyp.GetLeftRank());
				Hypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_left_trg->GetScore() + new_left_trg->GetScore());
				new_hyp.SetLeftRank(hyp.GetLeftRank()+1);
				new_hyp.SetLeftChild(new_left_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgLeft(new_left_trg->GetTrgLeft());
				} else {
					new_hyp.SetTrgRight(new_left_trg->GetTrgRight());
				}
				q.push(new_hyp);
			}
		}
		else{
			new_left_trg = HyperGraph::GetTrgSpan(l, hyp.GetCenter()-1, hyp.GetLeftRank()+1);
			if(new_left_trg) {
				old_left_trg = HyperGraph::GetTrgSpan(l,hyp.GetCenter()-1,hyp.GetLeftRank());
				Hypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_left_trg->GetScore() + new_left_trg->GetScore());
				new_hyp.SetLeftRank(hyp.GetLeftRank()+1);
				new_hyp.SetLeftChild(new_left_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgLeft(new_left_trg->GetTrgLeft());
				} else {
					new_hyp.SetTrgRight(new_left_trg->GetTrgRight());
				}
				q.push(new_hyp);
			}
		}
		child = dynamic_cast<DiscontinuousTargetSpan*>(hyp.GetRightChild());
		if (child != NULL){
			new_right_trg = GetTrgSpan(
					child->GetLeft(), child->GetM(),
					child->GetN(), child->GetRight(), hyp.GetRightRank()+1);
			if(new_right_trg) {
				old_right_trg = GetTrgSpan(
						child->GetLeft(), child->GetM(),
						child->GetN(), child->GetRight(), hyp.GetRightRank());
				Hypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_right_trg->GetScore() + new_right_trg->GetScore());
				new_hyp.SetRightRank(hyp.GetRightRank()+1);
				new_hyp.SetRightChild(new_right_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgRight(new_right_trg->GetTrgRight());
				} else {
					new_hyp.SetTrgLeft(new_right_trg->GetTrgLeft());
				}
				q.push(new_hyp);
			}
		}
		else{
			// Increment the right side if there is still a hypothesis right
	        new_right_trg = HyperGraph::GetTrgSpan(hyp.GetCenter(), r, hyp.GetRightRank()+1);
	        if(new_right_trg) {
	            old_right_trg = HyperGraph::GetTrgSpan(hyp.GetCenter(), r, hyp.GetRightRank());
				Hypothesis new_hyp(hyp);
				new_hyp.SetScore(hyp.GetScore()
						- old_right_trg->GetScore() + new_right_trg->GetScore());
				new_hyp.SetRightRank(hyp.GetRightRank()+1);
				new_hyp.SetRightChild(new_right_trg);
				if(new_hyp.GetType() == HyperEdge::EDGE_STR) {
					new_hyp.SetTrgRight(new_right_trg->GetTrgRight());
				} else {
					new_hyp.SetTrgLeft(new_right_trg->GetTrgLeft());
				}
				q.push(new_hyp);
			}
		}
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
