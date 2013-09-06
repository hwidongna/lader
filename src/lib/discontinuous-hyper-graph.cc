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
    Hypothesis * left, *right;
    if (cube_growing_){
    	left = left_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
    	right = right_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
    }
    else{
    	left = left_stack->GetHypothesis(0);
    	right = right_stack->GetHypothesis(0);
    }
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
    lm::ngram::Model::State out;
	if (bigram_)
		non_local_score += bigram_->Score(
				left->GetState(),
				bigram_->GetVocabulary().Index(
						sent[0]->GetElement(right->GetTrgLeft())),
				out);
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
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
	if (bigram_)
		non_local_score += bigram_->Score(
				right->GetState(),
				bigram_->GetVocabulary().Index(
						sent[0]->GetElement(left->GetTrgLeft())),
				out);
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
	if(left_stack == NULL) THROW_ERROR("Null left_trg "
			"["<<left_l<<", "<<left_m<<", "<<left_n<<", "<<left_r<<"]");
	if(right_stack == NULL) THROW_ERROR("Null right_trg "
			"["<<right_l<<", "<<right_m<<", "<<right_n<<", "<<right_r<<"]");
	int l, m, n, r, c;
	HyperEdge * edge;
    Hypothesis * left, *right;
    if (cube_growing_){
    	left = left_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
    	right = right_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
    }
    else{
    	left = left_stack->GetHypothesis(0);
    	right = right_stack->GetHypothesis(0);
    }
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
    lm::ngram::Model::State out;
	if (bigram_)
		non_local_score += bigram_->Score(
				left->GetState(),
				bigram_->GetVocabulary().Index(
						sent[0]->GetElement(right->GetTrgLeft())),
				out);
	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
	q.push(new DiscontinuousHypothesis(viterbi_score, score, non_local_score, edge,
			left->GetTrgLeft(), right->GetTrgRight(),
			l+1 == r ? out : right->GetState(),
			0, 0, left_stack, right_stack));
	// Add the inverted terminal
	edge = new DiscontinuousHyperEdge(l, m, c, n, r, HyperEdge::EDGE_INV);
	score = GetEdgeScore(model, features, sent, *edge);
	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
	if (bigram_)
		non_local_score += bigram_->Score(
				right->GetState(),
				bigram_->GetVocabulary().Index(
						sent[0]->GetElement(left->GetTrgLeft())),
				out);
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
    while((!beam_size || num_processed < beam_size) && q.size()){
        // Pop a hypothesis from the stack and get its target span
        Hypothesis *hyp = q.top(); q.pop();
        // If the next hypothesis on the stack is equal to the current
        // hypothesis, remove it, as this just means that we added the same
        // hypothesis
//        while(q.size() && *q.top() == *hyp){
//            delete q.top();
//            q.pop();
//        }
        // skip unnecessary hypothesis
        // Insert the hypothesis
        bool skip = hyp->CanSkip() || !ret->AddUniqueHypothesis(hyp);
        if (!skip){
        	num_processed++;
			if(verbose_ > 1){
				DiscontinuousHypothesis * dhyp =
						dynamic_cast<DiscontinuousHypothesis*>(hyp);
				cerr << "Insert " << num_processed << "th hypothesis ";
				if (dhyp) cerr << *dhyp;
				else cerr << *hyp;
				cerr << endl;

				const FeatureVectorInt *fvi = HyperGraph::GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
				FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
				FeatureVectorString *fws = model.StringifyWeightVector(*fvi);
				cerr << *fvs << endl << *fws;
				hyp->PrintChildren(cerr);
				delete fvs, fws;
				if(hyp->GetLeftChild() && hyp->GetRightChild()){
					const FeatureVectorInt *fvi;
					if(hyp->GetEdgeType() == HyperEdge::EDGE_STR)
						fvi = non_local_features.MakeNonLocalFeatures(sent, *hyp->GetLeftHyp(), *hyp->GetRightHyp(), model.GetFeatureIds(), model.GetAdd());
					else
						fvi = non_local_features.MakeNonLocalFeatures(sent, *hyp->GetRightHyp(), *hyp->GetLeftHyp(), model.GetFeatureIds(), model.GetAdd());

					fvs = model.StringifyFeatureVector(*fvi);
					fws = model.StringifyWeightVector(*fvi);
					cerr << "/********************* non-local features ***********************/" << endl;
					cerr << *fvs << endl << *fws;
					delete fvi, fvs, fws;
				}
				cerr << endl << "/****************************************************************/" << endl;
			}
        }

        // Add next cube items
    	Hypothesis * new_left = NULL, *new_right = NULL;

    	TargetSpan *left_span = hyp->GetLeftChild();
    	if (left_span)
    		new_left = left_span->GetHypothesis(hyp->GetLeftRank()+1);
    	hyp->IncrementLeft(new_left, model, non_local_features, sent, bigram_, q);

    	TargetSpan *right_span = hyp->GetRightChild();
    	if (right_span)
    		new_right = right_span->GetHypothesis(hyp->GetRightRank()+1);
    	hyp->IncrementRight(new_right, model, non_local_features, sent, bigram_, q);

    	if (skip)
    		delete hyp;
    }

    while(q.size()){
        delete q.top();
        q.pop();
    }
    sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
}

// Build a hypergraph using beam search and cube pruning
TargetSpan *DiscontinuousHyperGraph::ProcessOneDiscontinuousSpan(
		ReordererModel & model,
		const FeatureSet & features,
		const FeatureSet & non_local_features,
		const Sentence & sent,
		int l, int m, int n, int r,
		int beam_size){
    // Get a map to store identical target spans
	TargetSpan * ret = new DiscontinuousTargetSpan(l, m, n, r);
    HypothesisQueue * q;
	if (cube_growing_)
		q = &ret->GetCands();
    // Create the temporary data members for this span
	else
		q = new HypothesisQueue;

	double score, viterbi_score;
	if (verbose_ > 1)
		cerr << "Process ["<<l<<", "<<m<<", "<<n<<", "<<r<<"]" << endl;
	// continuous + continuous = discontinuous
	//cerr << "continuous + continuous = discontinuous" << endl;
	AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
			l, -1, -1, m,
			n, -1, -1, r);
	if (full_fledged_){
		// continuous + discontinuous = discontinuous
		//cerr << "continuous + discontinuous = discontinuous" << endl;
		for (int i = l+1 ; i <= m ; i++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
					l, -1, -1, i-1,
					i, m, n, r);
		// discontinuous + continuous = discontinuous
		//cerr << "discontinuous + continuous = discontinuous" << endl;
		for (int i = n+1 ; i <= r ; i++)
			AddDiscontinuousHypotheses(model, features, non_local_features, sent, *q,
					l, m, n, i-1,
					i, -1, -1, r);
	}

    // For cube growing, search is lazy
    if (cube_growing_)
    	return ret;

    StartBeamSearch(beam_size, *q, model, sent, features, non_local_features, ret, l, r);
    delete q;
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
    // Get a map to store identical target spans
	TargetSpan * ret = new TargetSpan(l, r);
    HypothesisQueue * q;
	if (cube_growing_)
		q = &ret->GetCands();
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
	        Hypothesis * left, *right;
	        if (cube_growing_){
	        	left = left_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
	        	right = right_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
	        }
	        else{
	        	left = left_stack->GetHypothesis(0);
	        	right = right_stack->GetHypothesis(0);
	        }
			// Add the straight non-terminal
			HyperEdge * edge = new HyperEdge(l, i, r, HyperEdge::EDGE_STR);
			score = GetEdgeScore(model, features, sent, *edge);
			double non_local_score = GetNonLocalScore(model, non_local_features, sent,
					*left, *right);
		    lm::ngram::Model::State out;
			if (bigram_)
				non_local_score += bigram_->Score(
						left->GetState(),
						bigram_->GetVocabulary().Index(
								sent[0]->GetElement(right->GetTrgLeft())),
						out);
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
				l, -1, -1, i-1,
				i, -1, -1, r);
		for (int d = 1 ; d <= D ; d++){
			// discontinuous + discontinuous = continuous
			//cerr << "discontinuous + discontinuous = continuous" << endl;
			for (int j = i+d+1 ; j <= r && j - (i + d) <= D ; j++){
				AddHypotheses(model, features, non_local_features, sent, *q,
						l, i-1, i+d, j-1,
						i, i-1+d, j, r);
			}
			// Process [0, m, n, N-1] is meaningless
			if ( r+d < N && r+d-l+1 != N){
				SetStack(l, i-1, i+d, r+d,
						ProcessOneDiscontinuousSpan(
								model, features, non_local_features, sent,
								l, i-1, i+d, r+d,
								beam_size));
			}
		}
	}

    // For cube growing, search is lazy
    if (cube_growing_)
    	return ret;

    StartBeamSearch(beam_size, *q, model, sent, features, non_local_features, ret, l, r);
    delete q;
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
            hyp.PrintChildren(cerr);
			FeatureVectorString * fvs = model.StringifyFeatureVector(*fvi);
			FeatureVectorString * fws = model.StringifyWeightVector(*fvi);
			cerr << "/********************* non-local features ***********************/" << endl;
			cerr << *fvs << endl << *fws << endl;
			cerr << "/****************************************************************/" << endl;
			delete fvs, fws;
		}
        BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
            feat_map[feat_pair.first] += feat_pair.second;
        delete fvi;
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
            		// Process [0, m, n, N-1] is meaningless
					if ( r+d < N && r+d-l+1 != N && GetStack(l,i-1,i+d,r+d) != NULL){
						if (verbose_ > 1)
							cerr << "AddLoss ["<<l<<", "<<i-1<<", "<<i+d<<", "<<r+d<<"]" << endl;
						BOOST_FOREACH(Hypothesis* hyp, GetStack(l,i-1,i+d,r+d)->GetHypotheses()) {
							// DEBUG
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
