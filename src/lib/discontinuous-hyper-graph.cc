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
	if(left_stack == NULL) THROW_ERROR("Null left_trg "
			"["<<left_l<<", "<<left_m<<", "<<left_n<<", "<<left_r<<"]");
	if(right_stack == NULL) THROW_ERROR("Null right_trg "
			"["<<right_l<<", "<<right_m<<", "<<right_n<<", "<<right_r<<"]");
	int l, m, c, n, r;
	HyperEdge * edge;
	Hypothesis * left= left_stack->GetHypothesis(0);
	Hypothesis * right = right_stack->GetHypothesis(0);
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

	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *left, *right);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
			left->GetTrgLeft(),
			right->GetTrgRight(),
			0, 0, left_stack, right_stack));
	// Add the inverted terminal
	if (m < 0 && n < 0)
		edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
	else
		edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
			right->GetTrgLeft(),
			left->GetTrgRight(),
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
	if(left_stack == NULL) THROW_ERROR("Null left_trg "
			"["<<left_l<<", "<<left_m<<", "<<left_n<<", "<<left_r<<"]");
	if(right_stack == NULL) THROW_ERROR("Null right_trg "
			"["<<right_l<<", "<<right_m<<", "<<right_n<<", "<<right_r<<"]");
	int l, m, n, r, c;
	HyperEdge * edge;
	Hypothesis * left= left_stack->GetHypothesis(0);
	Hypothesis * right = right_stack->GetHypothesis(0);
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
	// Add the straight terminal
	edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_STR);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *left, *right);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, non_local_score, edge,
			left->GetTrgLeft(),
			right->GetTrgRight(),
			0, 0, left_stack, right_stack));
	// Add the inverted terminal
	edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, non_local_score, edge,
			right->GetTrgLeft(),
			left->GetTrgRight(),
			0, 0, left_stack, right_stack));
}

void DiscontinuousHyperGraph::nextCubeItems(const Hypothesis * hyp,
		ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent,	HypothesisQueue & q,
		int l, int r, bool gap)
{
	Hypothesis * new_hyp,
	*new_left_hyp = NULL, *old_left_hyp = NULL,
	*new_right_hyp = NULL, *old_right_hyp = NULL;
	double non_local_score;
	TargetSpan *left_span = hyp->GetLeftChild();
	// Increment the left side if there is still a hypothesis left
	if (left_span)
		new_left_hyp = left_span->GetHypothesis(hyp->GetLeftRank()+1);
	if (new_left_hyp != NULL){
        old_left_hyp = hyp->GetLeftHyp();
        old_right_hyp = hyp->GetRightHyp();
		if (gap){
			DiscontinuousHyperEdge * edge =
					dynamic_cast<DiscontinuousHyperEdge*>(hyp->GetEdge());
			new_hyp = new DiscontinuousHypothesis(*hyp);
			new_hyp->SetEdge(edge->Clone());
		}
		else{
			new_hyp = new Hypothesis(*hyp);
			new_hyp->SetEdge(hyp->GetEdge()->Clone());
		}
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = GetNonLocalScore(model, non_local_features, sent,
											*new_left_hyp, *old_right_hyp);
		else
			non_local_score = GetNonLocalScore(model, non_local_features, sent,
											*old_right_hyp, *new_left_hyp);
		new_hyp->SetScore(hyp->GetScore()
				- old_left_hyp->GetScore() + new_left_hyp->GetScore()
				- old_left_hyp->GetNonLocalScore() + non_local_score);
		new_hyp->SetLeftRank(hyp->GetLeftRank() + 1);
		new_hyp->SetLeftChild(left_span);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgLeft(new_left_hyp->GetTrgLeft());
		else
			new_hyp->SetTrgRight(new_left_hyp->GetTrgRight());
		q.push(new_hyp);
	}
	TargetSpan *right_span = hyp->GetRightChild();
	// Increment the right side if there is still a hypothesis right
	if (right_span)
		new_right_hyp = right_span->GetHypothesis(hyp->GetRightRank()+1);
	if (new_right_hyp != NULL){
        old_left_hyp = hyp->GetLeftHyp();
        old_right_hyp = hyp->GetRightHyp();
		if (gap){
			DiscontinuousHyperEdge * edge =
					dynamic_cast<DiscontinuousHyperEdge*>(hyp->GetEdge());
			new_hyp = new DiscontinuousHypothesis(*hyp);
			new_hyp->SetEdge(edge->Clone());
		}
		else{
			new_hyp = new Hypothesis(*hyp);
			new_hyp->SetEdge(hyp->GetEdge()->Clone());
		}
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = GetNonLocalScore(model, non_local_features, sent,
											*old_left_hyp, *new_right_hyp);
		else
			non_local_score = GetNonLocalScore(model, non_local_features, sent,
											*new_right_hyp, *old_left_hyp);
		new_hyp->SetScore(hyp->GetScore()
				- old_right_hyp->GetScore() + new_right_hyp->GetScore()
				- old_right_hyp->GetNonLocalScore() + non_local_score);
		new_hyp->SetRightRank(hyp->GetRightRank()+1);
		new_hyp->SetRightChild(right_span);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgRight(new_right_hyp->GetTrgRight());
		else
			new_hyp->SetTrgLeft(new_right_hyp->GetTrgLeft());
		q.push(new_hyp);
	}
}

// Build a hypergraph using beam search and cube pruning
TargetSpan *DiscontinuousHyperGraph::ProcessOneDiscontinuousSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const FeatureSet & non_local_features,
		const Sentence & sent,
		int l, int m, int n, int r,
		int beam_size){
	HypothesisQueue q;
	double score, viterbi_score;
	if (verbose_ > 1)
		cerr << "Process ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	// continuous + continuous = discontinuous
	//cerr << "continuous + continuous = discontinuous" << endl;
	AddDiscontinuousHypotheses(model, features, non_local_features, sent, q,
			l, -1, -1, m,
			n, -1, -1, r);
	if (full_fledged_){
		// continuous + discontinuous = discontinuous
		//cerr << "continuous + discontinuous = discontinuous" << endl;
		for (int i = l+1 ; i <= m ; i++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, q,
					l, -1, -1, i-1,
					i, m, n, r);
		// discontinuous + continuous = discontinuous
		//cerr << "discontinuous + continuous = discontinuous" << endl;
		for (int i = n+1 ; i <= r ; i++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, q,
					l, m, n, i-1,
					i, -1, -1, r);
	}

	TargetSpan * ret = new DiscontinuousTargetSpan(l, m, n, r);
	// Start beam search
	//cerr << "Start beam search among " << q.size() << " hypotheses" << endl;
	int num_processed = 0;
	while((!beam_size || num_processed < beam_size) && q.size()) {
		// Pop a hypothesis from the stack and get its target span
		DiscontinuousHypothesis * hyp =
				dynamic_cast<DiscontinuousHypothesis*>(q.top()); q.pop();
		// Insert the hypothesis
		if (verbose_ > 1){
			cerr << "Insert the discontinuous hypothesis " << *hyp << endl;
			const FeatureVectorInt * fvi = HyperGraph::GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			FeatureVectorString * fws = model.StringifyWeightVector(*fvi);
			cerr << *fvs << endl << *fws;
			hyp->PrintChildren(cerr);
			delete fvs, fws;
			if (hyp->GetLeftChild() && hyp->GetRightChild()){
				const FeatureVectorInt * fvi;
				if (hyp->GetEdgeType() == HyperEdge::EDGE_STR)
					fvi = non_local_features.MakeNonLocalFeatures(
									sent, *hyp->GetLeftHyp(), *hyp->GetRightHyp(), model.GetFeatureIds(), model.GetAdd());
				else
					fvi = non_local_features.MakeNonLocalFeatures(
									sent, *hyp->GetRightHyp(), *hyp->GetLeftHyp(), model.GetFeatureIds(), model.GetAdd());
				fvs = model.StringifyFeatureVector(*fvi);
				fws = model.StringifyWeightVector(*fvi);
				cerr << "/*************** non-local features *******************/" << endl;
				cerr << *fvs << endl << *fws << endl;
				cerr << "/******************************************************/" << endl;
				delete fvs, fws;
			}
		}
		ret->AddHypothesis(hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && *q.top() == *hyp) {
			delete q.top();
			q.pop();
		}
		// Skip terminals
//		if(hyp->GetCenter() == -1) continue;
		// Increment the left side if there is still a hypothesis left
		nextCubeItems(hyp, model, features, non_local_features, sent, q, l, r, true);
	}
	while(q.size()) {
		delete q.top();
		q.pop();
	}
	sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
	return ret;
}

// Build a discontinuous hypergraph using beam search and cube pruning
// Override HyperGraph::ProcessOneSpan
TargetSpan * DiscontinuousHyperGraph::ProcessOneSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const FeatureSet & non_local_features,
		const Sentence & sent,
		int l, int r,
		int beam_size) {
	// Create the temporary data members for this span
	HypothesisQueue q;
	double score;
	// If the length is OK, add a terminal
	if((features.GetMaxTerm() == 0) || (r-l < features.GetMaxTerm())) {
		// Create a hypothesis with the forward terminal
		HyperEdge * edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_FOR);
		score = GetEdgeScore(model, features, sent, *edge);
		q.push(new Hypothesis(score, score, 0, edge, l, r));
		if(features.GetUseReverse()) {
			// Create a hypothesis with the backward terminal
			edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_BAC);
			score = GetEdgeScore(model, features, sent, *edge);
			q.push(new Hypothesis(score, score, 0, edge, r, l));
		}
	}

    bool hasPunct = false;
    for (int i = l; i <= r; i++ )
    	hasPunct |= sent[0]->isPunct(i);

	int N = n_ = sent[0]->GetNumWords();
	int D = gap_;
	if (verbose_ > 1)
		cerr << "Process ["<<l<<", "<<r<<"]" << endl;
	for (int i = l+1 ; i <= r ; i++){
		if (hasPunct && mp_){ // monotone at punctuation
			TargetSpan *left_stack, *right_stack;
			left_stack = HyperGraph::GetStack(l, i-1);
			right_stack = HyperGraph::GetStack(i, r);
			if(left_stack == NULL) THROW_ERROR("Target l="<<l<<", c-1="<<i-1);
			if(right_stack == NULL) THROW_ERROR("Target c="<<i<<", r="<<r);
			// Add the straight non-terminal
			HyperEdge * edge = new HyperEdge(l, i, r, HyperEdge::EDGE_STR);
			score = GetEdgeScore(model, features, sent, *edge);
			double non_local_score;
			non_local_score = GetNonLocalScore(model, non_local_features, sent,
					*left_stack->GetHypothesis(0), *right_stack->GetHypothesis(0));
			double viterbi_score = score + non_local_score +
					left_stack->GetScore() + right_stack->GetScore();
			q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
					left_stack->GetHypothesis(0)->GetTrgLeft(),
					right_stack->GetHypothesis(0)->GetTrgRight(),
					0, 0, left_stack, right_stack));
			continue;
		}
		// continuous + continuous = continuous
		//cerr << "continuous + continuous = continuous" << endl;
		AddHypotheses(model, features, non_local_features, sent, q,
				l, -1, -1, i-1,
				i, -1, -1, r);
		for (int d = 1 ; d <= D ; d++){
			// discontinuous + discontinuous = continuous
			//cerr << "discontinuous + discontinuous = continuous" << endl;
			for (int j = i+d+1 ; j <= r && j - (i + d) <= D ; j++){
				AddHypotheses(model, features, non_local_features, sent, q,
						l, i-1, i+d, j-1,
						i, i-1+d, j, r);
			}
			if ( r + d < N){
				SetStack(l, i-1, i+d, r+d,
						ProcessOneDiscontinuousSpan(
								model, features, non_local_features, sent,
								l, i-1, i+d, r+d,
								beam_size));
			}
		}
	}

	TargetSpan * ret = new TargetSpan(l, r);
	// Start beam search
	int num_processed = 0;
	while((!beam_size || num_processed < beam_size) && q.size()) {
		// Pop a hypothesis from the stack and get its target span
		Hypothesis * hyp = q.top(); q.pop();
		// Insert the hypothesis
		if (verbose_ > 1){
			cerr << "Insert the hypothesis " << *hyp << endl;
			const FeatureVectorInt * fvi = HyperGraph::GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			FeatureVectorString * fws = model.StringifyWeightVector(*fvi);
			cerr << *fvs << endl << *fws;
			if (hyp->GetCenter() == -1)
				cerr << endl;
			else
				hyp->PrintChildren(cerr);
			delete fvs, fws;
			if (hyp->GetLeftChild() && hyp->GetRightChild()){
				const FeatureVectorInt * fvi;
				if (hyp->GetEdgeType() == HyperEdge::EDGE_STR)
					fvi = non_local_features.MakeNonLocalFeatures(
									sent, *hyp->GetLeftHyp(), *hyp->GetRightHyp(), model.GetFeatureIds(), model.GetAdd());
				else
					fvi = non_local_features.MakeNonLocalFeatures(
									sent, *hyp->GetRightHyp(), *hyp->GetLeftHyp(), model.GetFeatureIds(), model.GetAdd());
				fvs = model.StringifyFeatureVector(*fvi);
				fws = model.StringifyWeightVector(*fvi);
				cerr << "/********************* non-local features ***********************/" << endl;
				cerr << *fvs << endl << *fws << endl;
				cerr << "/****************************************************************/" << endl;
				delete fvs, fws;
			}
		}
		ret->AddHypothesis(hyp);
		num_processed++;
		// If the next hypothesis on the stack is equal to the current
		// hypothesis, remove it, as this just means that we added the same
		// hypothesis
		while(q.size() && *q.top() == *hyp) {
			delete q.top();
			q.pop();
		}
		// Skip terminals
//		if(hyp->GetCenter() == -1) continue;
		nextCubeItems(hyp, model, features, non_local_features, sent, q, l, r, false);
	}
	while(q.size()) {
		delete q.top();
		q.pop();
	}
	sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
	return ret;
}

// Accumulate non-local features under a hypothesis
void DiscontinuousHyperGraph::AccumulateNonLocalFeatures(std::tr1::unordered_map<int,double> & feat_map,
										ReordererModel & model,
		                                const FeatureSet & feature_gen,
		                                const Sentence & sent,
		                                const Hypothesis & hyp){
	Hypothesis * left = hyp.GetLeftHyp();
	Hypothesis * right = hyp.GetRightHyp();
	// terminals have no non-local features
	if (!left && !right)
		return;
	// the root has no non-local features
	if (hyp.GetEdgeType() != HyperEdge::EDGE_ROOT){
		const FeatureVectorInt * fvi;
		if (hyp.GetEdgeType() == HyperEdge::EDGE_STR)
			fvi = feature_gen.MakeNonLocalFeatures(sent, *left, *right, model.GetFeatureIds(), model.GetAdd());
		else if (hyp.GetEdgeType() == HyperEdge::EDGE_INV)
			fvi = feature_gen.MakeNonLocalFeatures(sent, *right, *left, model.GetFeatureIds(), model.GetAdd());
		if (verbose_ > 1){
			cerr << "Accumulate non-local feature of hypothesis " << hyp << endl;
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			FeatureVectorString * fws = model.StringifyWeightVector(*fvi);
			cerr << "/********************* non-local features ***********************/" << endl;
			cerr << *fvs << endl << *fws << endl;
			cerr << "/****************************************************************/" << endl;
			delete fvs, fws;
		}
        BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
            feat_map[feat_pair.first] += feat_pair.second;
	}
	if (left)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *left);
	if (right)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *right);
}

void DiscontinuousHyperGraph::AddLoss(LossBase* loss,
		const Ranks * ranks, const FeatureDataParse * parse) const{
    // Initialize the loss
    loss->Initialize(ranks, parse);
    // For each span in the hypergraph
    int N = n_;
    int D = gap_;
    if (verbose_ > 1){
    	cerr << "Rank:";
    	BOOST_FOREACH(int rank, ranks->GetRanks())
    		cerr << " " << rank;
    	cerr << endl;
    }
    for(int r = 0; r <= N; r++) {
        // When r == n, we want the root, so only do -1
        for(int l = (r == N ? -1 : 0); l <= (r == N ? -1 : r); l++) {
            // DEBUG
        	if (verbose_ > 1)
        		cerr << "AddLoss ["<<l<<", "<<r<<"]" << endl;
        	BOOST_FOREACH(Hypothesis* hyp, HyperGraph::GetStack(l,r)->GetHypotheses()) {
        		// DEBUG
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
            for (int i = l+1 ; i <= r ; i++){
            	for (int d = 1 ; d <= D ; d++){
					if ( i+d <= r && r+d < N && GetStack(l,i-1,i+d,r) != NULL){
						if (verbose_ > 1)
							cerr << "AddLoss ["<<l<<", "<<i-1<<", "<<i+d<<", "<<r<<"]" << endl;
						BOOST_FOREACH(Hypothesis* hyp, GetStack(l,i-1,i+d,r)->GetHypotheses()) {
							// DEBUG
							hyp->SetLoss(hyp->GetLoss() +
									loss->AddLossToProduction(hyp, ranks, parse));
							if (verbose_ > 1){
								cerr << "Loss=" << hyp->GetLoss() << ":" << *hyp;
								hyp->PrintChildren(cerr);
							}
						}
					}
            	}
        	}
        }
    }
}
