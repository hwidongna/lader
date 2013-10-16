#include <lader/hyper-edge.h>
#include <lader/hyper-graph.h>
#include <lader/feature-vector.h>
#include <lader/feature-set.h>
#include <lader/reorderer-model.h>
#include <lader/target-span.h>
#include <lader/discontinuous-hypothesis.h>
#include <lader/hypothesis-queue.h>
#include <lader/util.h>
#include <tr1/unordered_map>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <lader/thread-pool.h>
#include <boost/thread/mutex.hpp>

using namespace lader;
using namespace std;
using namespace boost;

// Make biligual features is possible
FeatureVectorInt *HyperGraph::MakeEdgeFeatures(const FeatureSet & feature_gen,
		const Sentence & source, const HyperEdge & edge, ReordererModel & model)
{
    FeatureVectorInt *ret;
    ret = feature_gen.MakeEdgeFeatures(source, edge, model.GetFeatureIds(), model.GetAdd());
    if(bilingual_features_ && target_ && align_)
        bilingual_features_->MakeBilingualFeatures(source, *target_, *align_,
				edge, model.GetFeatureIds(), model.GetAdd(), ret);
    return ret;
}
// Return the edge feature vector
// if -save_features, edge features are stored in a stack.
// thus, this is thread safe without lock.
const FeatureVectorInt * HyperGraph::GetEdgeFeatures(
                                ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge) {
	FeatureVectorInt * ret;
    if (save_features_) {
    	TargetSpan * stack = GetStack(edge.GetLeft(), edge.GetRight());
    	switch (edge.GetType()){
    	case HyperEdge::EDGE_FOR:
    	case HyperEdge::EDGE_STR:
    		ret = stack->GetStraightFeautures(edge.GetCenter() - edge.GetLeft());
    		if (ret == NULL){
    			ret = MakeEdgeFeatures(feature_gen, sent, edge, model);
    			stack->SaveStraightFeautures(edge.GetCenter() - edge.GetLeft(), ret);
    		}
    		break;
    	case HyperEdge::EDGE_BAC:
    	case HyperEdge::EDGE_INV:
    		ret = stack->GetInvertedFeautures(edge.GetCenter() - edge.GetLeft());
    		if (ret == NULL){
    			ret = MakeEdgeFeatures(feature_gen, sent, edge, model);
    			stack->SaveInvertedFeautures(edge.GetCenter() - edge.GetLeft(), ret);
    		}
    		break;
    	case HyperEdge::EDGE_ROOT:
    	default:
    		THROW_ERROR("Invalid hyper edge for GetEdgeFeatures: " << edge);
    	}
    } else {
    	ret = MakeEdgeFeatures(feature_gen, sent, edge, model);
    }
    return ret;
}

double HyperGraph::Score(double loss_multiplier,
                         const Hypothesis* hyp) const{
    double score = hyp->GetLoss()*loss_multiplier;
    if(hyp->GetEdgeType() != HyperEdge::EDGE_ROOT) {
    	score += hyp->GetSingleScore();
    }
    if(hyp->GetLeftChild())
		score += Score(loss_multiplier, hyp->GetLeftHyp());
	if(hyp->GetRightChild())
		score += Score(loss_multiplier, hyp->GetRightHyp());
	return score;
}

double HyperGraph::Rescore(double loss_multiplier) {
    // Score all root hypotheses, and place the best hyp at hyps[0].
	// Note that it does not modify the rest of hypotheses under the root.
    // Therefore, this keep the forest structure by BuildHyperGraph
    std::vector<Hypothesis*> & hyps = GetRoot()->GetHypotheses();
    for(int i = 0; i < (int)hyps.size(); i++) {
        hyps[i]->SetScore(Score(loss_multiplier, hyps[i]));
        if(hyps[i]->GetScore() > hyps[0]->GetScore())
            swap(hyps[i], hyps[0]);
    }
    return hyps[0]->GetScore();
}

// Get the score for a single edge
double HyperGraph::GetEdgeScore(ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & source,
                                const HyperEdge & edge) {
    const FeatureVectorInt * vec = 
                GetEdgeFeatures(model, feature_gen, source, edge);
    double score = model.ScoreFeatureVector(SafeReference(vec));
    // features are not stored, thus delete
    if (!save_features_)
    	delete vec;
    return score;
}

// Accumulate edge features under a hyper-edge
void HyperGraph::AccumulateFeatures(std::tr1::unordered_map<int,double> & feat_map,
										ReordererModel & model,
		                                const FeatureSet & features,
		                                const Sentence & sent,
		                                const Hypothesis * hyp){
	Hypothesis * left = hyp->GetLeftHyp();
	Hypothesis * right = hyp->GetRightHyp();
	// root has no edge feature
	if (hyp->GetEdgeType() != HyperEdge::EDGE_ROOT){
		// accumulate edge features
		const FeatureVectorInt * fvi = GetEdgeFeatures(model, features, sent, *hyp->GetEdge());
		BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
			feat_map[feat_pair.first] += feat_pair.second;
		// if fvi is not stored, delete
		if (!save_features_)
			delete fvi;
	}
	if (left)
		AccumulateFeatures(feat_map, model, features, sent, left);
	if (right)
		AccumulateFeatures(feat_map, model, features, sent, right);
}

// If the length is OK, add a terminal
void HyperGraph::AddTerminals(int l, int r, const FeatureSet & features, ReordererModel & model, const Sentence & sent, HypothesisQueue *& q)
{
    // If the length is OK, add a terminal
    if((model.GetMaxTerm() == 0) || (r - l < model.GetMaxTerm())){
        double score;
        // Create a hypothesis with the forward terminal
        HyperEdge *edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_FOR);
        score = GetEdgeScore(model, features, sent, *edge);
        q->push(new Hypothesis(score, score, edge, l, r));
        if(model.GetUseReverse()){
            edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_BAC);
            score = GetEdgeScore(model, features, sent, *edge);
            q->push(new Hypothesis(score, score, edge, r, l));
        }
    }


}


// Increment the left side if there is still a hypothesis left
void HyperGraph::IncrementLeft(const Hypothesis *old_hyp,
		const Hypothesis *new_left, ReordererModel & model,
		const Sentence & sent,
		HypothesisQueue & q, int & pop_count)
{
   if (new_left != NULL){
	   Hypothesis * old_left  = old_hyp->GetLeftHyp();
	   Hypothesis * old_right = old_hyp->GetRightHyp();
	   if (!old_left || !old_right)
		   THROW_ERROR("Fail to IncrementLeft for " << *old_hyp << endl)
        // Clone this hypothesis
		Hypothesis * new_hyp = old_hyp->Clone();
		new_hyp->SetScore(old_hyp->GetScore()
				- old_left->GetScore() + new_left->GetScore());
		new_hyp->SetLeftRank(old_hyp->GetLeftRank()+1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgLeft(new_left->GetTrgLeft());
		else
			new_hyp->SetTrgRight(new_left->GetTrgRight());
		if (new_hyp->GetTrgLeft() < new_hyp->GetLeft()
		|| new_hyp->GetTrgRight() < new_hyp->GetLeft()
		|| new_hyp->GetRight() < new_hyp->GetTrgLeft()
		|| new_hyp->GetRight() < new_hyp->GetTrgRight()) {
			new_hyp->PrintParse(cerr);
			DiscontinuousHypothesis * dhyp =
					dynamic_cast<DiscontinuousHypothesis*>(new_hyp);
			THROW_ERROR("Malformed hypothesis in IncrementLeft " << (dhyp? *dhyp : *new_hyp) << endl)
		}
		q.push(new_hyp);
	}
}

// Increment the right side if there is still a hypothesis right
void HyperGraph::IncrementRight(const Hypothesis *old_hyp,
		const Hypothesis *new_right, ReordererModel & model,
		const Sentence & sent,
		HypothesisQueue & q, int & pop_count)
{
    if (new_right != NULL){
    	Hypothesis * old_left  = old_hyp->GetLeftHyp();
    	Hypothesis * old_right = old_hyp->GetRightHyp();
    	if (!old_left || !old_right)
    		THROW_ERROR("Fail to IncrementRight for " << *old_hyp << endl)
        // Clone this hypothesis
        Hypothesis * new_hyp = old_hyp->Clone();
		double non_local_score = 0.0;
		new_hyp->SetScore(old_hyp->GetScore()
				- old_right->GetScore() + new_right->GetScore());
		new_hyp->SetRightRank(old_hyp->GetRightRank()+1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgRight(new_right->GetTrgRight());
		else
			new_hyp->SetTrgLeft(new_right->GetTrgLeft());
		if (new_hyp->GetTrgLeft() < new_hyp->GetLeft()
		|| new_hyp->GetTrgRight() < new_hyp->GetLeft()
		|| new_hyp->GetRight() < new_hyp->GetTrgLeft()
		|| new_hyp->GetRight() < new_hyp->GetTrgRight()) {
			new_hyp->PrintParse(cerr);
			DiscontinuousHypothesis * dhyp =
					dynamic_cast<DiscontinuousHypothesis*>(new_hyp);
			THROW_ERROR("Malformed hypothesis in IncrementRight " << (dhyp? *dhyp : *new_hyp) << endl)
		}
		q.push(new_hyp);
	}
}

// For cube growing
void HyperGraph::LazyNext(HypothesisQueue & q, ReordererModel & model,
		const Sentence & sent,
		const Hypothesis * hyp, int & pop_count) {
	Hypothesis * new_left = NULL, *new_right = NULL;

	TargetSpan *left_span = hyp->GetLeftChild();
	if (left_span)
		new_left = LazyKthBest(left_span, hyp->GetLeftRank() + 1,
				model, sent, pop_count);
	IncrementLeft(hyp, new_left, model, sent, q, pop_count);

	TargetSpan *right_span = hyp->GetRightChild();
	if (right_span)
		new_right = LazyKthBest(right_span, hyp->GetRightRank() + 1,
				model, sent, pop_count);
	IncrementRight(hyp, new_right, model, sent, q, pop_count);
}

// For cube growing
Hypothesis * HyperGraph::LazyKthBest(TargetSpan * stack, int k,
		ReordererModel & model,
		const Sentence & sent, int & pop_count) {
	while (((!beam_size_ && stack->HypSize() < k + 1)
			|| stack->HypSize() < std::min(k + 1, beam_size_))
			&& stack->CandSize() > 0) {
		HypothesisQueue & q = stack->GetCands();
		Hypothesis * hyp = q.top(); q.pop();
		// Insert the hypothesis if unique
		bool skip = !stack->AddUniqueHypothesis(hyp);
		LazyNext(q, model, sent, hyp, pop_count);
		if (skip)
			delete hyp;
		if (pop_limit_ && ++pop_count > pop_limit_)
			break;
	}
	return stack->GetHypothesis(k);

}

// Build a hypergraph using beam search and cube pruning
void HyperGraph::ProcessOneSpan(ReordererModel & model,
                                       const FeatureSet & features,
                                       const Sentence & source,
                                       int l, int r,
                                       int beam_size) {
    // Get a map to store identical target spans
    TargetSpan * stack = GetStack(l, r);// new TargetSpan(l, r);
    if (!stack)
    	THROW_ERROR("SetStack is required: [" << l << ", " << r << "]")
	stack->ClearHypotheses();
    HypothesisQueue * q;
	if (cube_growing_)
		q = &stack->GetCands();

    else
        // Create the temporary data members for this span
        q = new HypothesisQueue;

    double score, viterbi_score;
    // If the length is OK, add a terminal
    AddTerminals(l, r, features, model, source, q);

    TargetSpan *left_stack, *right_stack;
    Hypothesis *new_left_hyp, *old_left_hyp,
               *new_right_hyp, *old_right_hyp;
    // Add the best hypotheses for each non-terminal to the queue
    for(int c = l+1; c <= r; c++) {
        // Find the best hypotheses on the left and right side
        left_stack = GetStack(l, c-1);
        right_stack = GetStack(c, r);
        if(left_stack == NULL) THROW_ERROR("Target l="<<l<<", c-1="<<c-1);
        if(right_stack == NULL) THROW_ERROR("Target c="<<c<<", r="<<r);
        Hypothesis * left, *right;
       	left = left_stack->GetHypothesis(0);
       	right = right_stack->GetHypothesis(0);
        // Add the straight terminal
        HyperEdge * edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
        score = GetEdgeScore(model, features, source, *edge);
    	viterbi_score = score + left->GetScore() + right->GetScore();
    	q->push(new Hypothesis(viterbi_score, score, edge,
                         left->GetTrgLeft(),
                         right->GetTrgRight(),
                         0, 0, left_stack, right_stack));
        // Add the inverted terminal
        edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
        score = GetEdgeScore(model, features, source, *edge);
    	viterbi_score = score + left->GetScore() + right->GetScore();
    	q->push(new Hypothesis(viterbi_score, score, edge,
						 right->GetTrgLeft(),
						 left->GetTrgRight(),
                         0, 0, left_stack, right_stack));

    }
    // For cube growing, search is lazy
    if (cube_growing_)
    	return;

    // Start beam search
    int num_processed = 0;
    int pop_count = 0;
    while((!beam_size || num_processed < beam_size) && q->size()) {
        // Pop a hypothesis from the stack and get its target span
        Hypothesis * hyp = q->top(); q->pop();
        // Insert the hypothesis if unique
        bool skip = !stack->AddUniqueHypothesis(hyp);
        if (!skip)
        	num_processed++;
        // Skip terminals
        if(hyp->GetCenter() == -1) continue;

    	Hypothesis * new_left = NULL, *new_right = NULL;

    	TargetSpan *left_span = hyp->GetLeftChild();
    	if (left_span)
    		new_left = left_span->GetHypothesis(hyp->GetLeftRank()+1);
    	IncrementLeft(hyp, new_left, model, source, *q, pop_count);

    	TargetSpan *right_span = hyp->GetRightChild();
    	if (right_span)
    		new_right = right_span->GetHypothesis(hyp->GetRightRank()+1);
    	IncrementRight(hyp, new_right, model, source, *q, pop_count);
        if (skip)
        	delete hyp;
    }
    while(!q->empty()){
        delete q->top();
        q->pop();
    }
    sort(stack->GetHypotheses().begin(), stack->GetHypotheses().end(), DescendingScore<Hypothesis>());
    delete q;
}

// for cube growing, trigger the best hypothesis
void HyperGraph::TriggerTheBestHypotheses(int l, int r, ReordererModel & model,
		const Sentence & sent) {
	int pop_count = 0;
	Hypothesis * best = LazyKthBest(GetStack(l, r), 0, model,
			sent, pop_count);
	if (!best)
		THROW_ERROR("Fail to produce hypotheses " << *GetStack(l, r) << endl);
}
// Build a hypergraph using beam search and cube pruning
void HyperGraph::BuildHyperGraph(ReordererModel & model,
        const FeatureSet & features,
        const Sentence & source) {
    SetNumWords(source[0]->GetNumWords());
    // if -save_features=true, reuse the stacks storing edge features
    if (!save_features_)
    	SetAllStacks();
    // Iterate through the left side of the span
    for(int L = 1; L <= n_; L++) {
        // Move the span from l to r, building hypotheses from small to large
    	// parallelize processing in a row
    	ThreadPool pool(threads_, 1000);
    	for(int l = 0; l <= n_-L; l++){
    		int r = l+L-1;
    		Task * task = NewSpanTask(this, model, features,
					source, l, r, beam_size_);
    		pool.Submit(task);
    	}
    	pool.Stop(true);
    	// for cube growing, trigger the best hypothesis
    	if (cube_growing_)
    		for(int l = 0; l <= n_-L; l++)
    			TriggerTheBestHypotheses(l, l+L-1, model, source);
    }
    // Build the root node
    TargetSpan * top = GetStack(0,n_-1);
    TargetSpan * root_stack = GetStack(0, n_);
    if (!root_stack)
    	THROW_ERROR("Either -save_features=false or InitStacks first")
    root_stack->ClearHypotheses();
    for(int i = 0; !beam_size_ || i < beam_size_; i++){
    	Hypothesis * hyp = NULL;
    	if (cube_growing_){
    		int pop_count = 0;
    		hyp = LazyKthBest(top, i, model, source, pop_count);
    	}
    	else if (i < (int)(top->HypSize()))
            hyp = (*top)[i];

		if(!hyp)
			break;

        root_stack->AddHypothesis(new Hypothesis(
        		hyp->GetScore() , 0,
        		new HyperEdge(0, -1, n_-1, HyperEdge::EDGE_ROOT),
        		hyp->GetTrgLeft(), hyp->GetTrgRight(),
                i, -1,
                top, NULL));
    }
}


void HyperGraph::AddLoss(LossBase* loss,
		const Ranks * ranks, const FeatureDataParse * parse) const{
    // Initialize the loss
    loss->Initialize(ranks, parse);
    // For each span in the hypergraph
    int n = n_;
    for(int r = 0; r <= n; r++) {
        // When r == n, we want the root, so only do -1
        for(int l = (r == n ? -1 : 0); l <= (r == n ? -1 : r); l++) {
            // DEBUG cerr << "l=" << l << ", r=" << r << ", n=" << n << endl;
			BOOST_FOREACH(Hypothesis* hyp, GetStack(l,r)->GetHypotheses()) {
				// DEBUG cerr << "GetLoss = " <<hyp->GetLoss()<<endl;
				hyp->SetLoss(hyp->GetLoss() +
							loss->AddLossToProduction(hyp, ranks, parse));
			}
        }
    }
}


template <class T>
inline string GetNodeString(char type, const T * hyp) {
    if(!hyp) return "";
    ostringstream oss;
    oss << type << "-" << hyp->GetLeft() << "-" << hyp->GetRight();
    return oss.str(); 
}

// Print the whole hypergraph
void HyperGraph::PrintHyperGraph(const std::vector<std::string> & strs,
                                 std::ostream & out) {
    SymbolSet<int> nodes, rules;
    vector<vector<string> > node_strings;
    // Reset the node IDs
    BOOST_FOREACH(TargetSpan * stack, stacks_)
		stack->ResetId();
    // Add the node IDs
    int last_id = 0;
    GetRoot()->LabelWithIds(last_id);
    // For each ending point of a span
    set<char> null_set; null_set.insert(0);
    for(int j = 0; j <= (int)strs.size(); j++) {
        // For each starting point of a span
        bool root = j==(int)strs.size();
        for(int i = (root?-1:j); i >= (root?-1:0); i--) {
            TargetSpan * stack = GetStack(i, j);
            if(stack->GetId() == -1)
            	continue;
            // For each hypothesis
			BOOST_FOREACH(const Hypothesis * hyp, stack->GetHypotheses()) {
				stack->SetHasType(hyp->GetEdgeType());
				int top_id = nodes.GetId(GetNodeString(hyp->GetEdgeType(), hyp),true);
				if((int)node_strings.size() <= top_id)
					node_strings.resize(top_id+1);
				TargetSpan *left_child = hyp->GetLeftChild();
				TargetSpan *right_child = hyp->GetRightChild();
				// For each type in the left
				BOOST_FOREACH(char left_type,
							  left_child ? left_child->GetHasTypes() : null_set) {
					int left_id = nodes.GetId(GetNodeString(left_type, left_child));
					// For each type in the right
					BOOST_FOREACH(char right_type,
								  right_child ? right_child->GetHasTypes() : null_set) {
						int right_id = nodes.GetId(GetNodeString(right_type, right_child));
						int rule_id = 1 + rules.GetId(hyp->GetRuleString(strs, left_type, right_type), true);
						ostringstream rule_oss;
						rule_oss << "{";
						if(left_id != -1) {
							rule_oss << "\"tail\":[" << left_id;
							if(right_id != -1)
								rule_oss << "," << right_id;
							rule_oss<<"],";
						}
						rule_oss << "\"feature\":{\"parser\":" << hyp->GetSingleScore()<<"},"
								<< "\"rule\":" << rule_id << "}";
						node_strings[top_id].push_back(rule_oss.str());
					}
				}
			}
            // We only need one time for the root
            if(i == -1) break;
        }
    }
    const vector<string*> & rule_vocab = rules.GetSymbols();
    out << "{\"rules\": [";
    for(int i = 0; i < (int)rule_vocab.size(); i++) {
        if(i != 0) out << ", ";
        out << "\"" << *rule_vocab[i] << "\"";
    }
    out << "], \"nodes\": [";
    for(int i = 0; i < (int)node_strings.size(); i++) {
        if(i != 0) out << ", ";
        out << "[" << algorithm::join(node_strings[i], ", ") << "]";
    }
    out << "], \"goal\": " << node_strings.size()-1 << "}";

}
