#include <lader/hyper-edge.h>
#include <lader/hyper-graph.h>
#include <lader/hypothesis-queue.h>
#include <lader/feature-vector.h>
#include <lader/feature-set.h>
#include <lader/reorderer-model.h>
#include <lader/target-span.h>
#include <lader/util.h>
#include <tr1/unordered_map>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace lader;
using namespace std;
using namespace boost;

// Return the edge feature vector
const FeatureVectorInt * HyperGraph::GetEdgeFeatures(
                                ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge) {
    FeatureVectorInt * ret;
    if(features_ == NULL) features_ = new EdgeFeatureMap;
    EdgeFeatureMap::const_iterator it = features_->find(&edge);
    if(it == features_->end()) {
        ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
        features_->insert(MakePair(edge.Clone(), ret));
    } else {
        ret = it->second;
    }
    return ret;
}

double HyperGraph::Score(double loss_multiplier,
                         const Hypothesis* hyp) const{
    double score = hyp->GetLoss()*loss_multiplier;
    if(hyp->GetEdgeType() != HyperEdge::EDGE_ROOT) {
    	score += hyp->GetSingleScore() + hyp->GetNonLocalScore();
    }
    if(hyp->GetLeftChild())
		score += Score(loss_multiplier, hyp->GetLeftHyp());
	if(hyp->GetRightChild())
		score += Score(loss_multiplier, hyp->GetRightHyp());
	return score;
}

double HyperGraph::Rescore(double loss_multiplier) {
    // Score from root for all hypotheses, while keeping the best hyp at hyps[0]
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
                                const Sentence & sent,
                                const HyperEdge & edge) {
    const FeatureVectorInt * vec = 
                GetEdgeFeatures(model, feature_gen, sent, edge);
    return model.ScoreFeatureVector(SafeReference(vec));
}

// Get the score for the left and right children of a hypothesis
double HyperGraph::GetNonLocalScore(ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const Hypothesis & left,
                                const Hypothesis & right) {
    // unlike edge features, do not store non-local features
    const FeatureVectorInt * vec =
    		feature_gen.MakeNonLocalFeatures(sent, left, right, model.GetFeatureIds(), model.GetAdd());
    return model.ScoreFeatureVector(SafeReference(vec));
}

// Accumulate non-local features under a hypothesis
void HyperGraph::AccumulateNonLocalFeatures(std::tr1::unordered_map<int,double> & feat_map,
										ReordererModel & model,
		                                const FeatureSet & feature_gen,
		                                const Sentence & sent,
		                                const Hypothesis & hyp){
	Hypothesis * left = hyp.GetLeftHyp();
	Hypothesis * right = hyp.GetRightHyp();
	// terminals have no non-local features
	if (!left || !right)
		return;
	// root has no non-local features
	if (hyp.GetEdgeType() != HyperEdge::EDGE_ROOT){
		const FeatureVectorInt * vec;
		if (hyp.GetEdgeType() == HyperEdge::EDGE_STR)
			vec = feature_gen.MakeNonLocalFeatures(sent, *left, *right, model.GetFeatureIds(), model.GetAdd());
		else if (hyp.GetEdgeType() == HyperEdge::EDGE_INV)
			vec = feature_gen.MakeNonLocalFeatures(sent, *right, *left, model.GetFeatureIds(), model.GetAdd());
        BOOST_FOREACH(FeaturePairInt feat_pair, *vec)
            feat_map[feat_pair.first] += feat_pair.second;
	}
	if (left)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *left);
	if (right)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *right);
}
// Build a hypergraph using beam search and cube pruning
TargetSpan * HyperGraph::ProcessOneSpan(ReordererModel & model,
                                       const FeatureSet & features,
                                       const FeatureSet & non_local_features,
                                       const Sentence & sent,
                                       int l, int r,
                                       int beam_size) {
    // Create the temporary data members for this span
    HypothesisQueue q;
    double score, viterbi_score, non_local_score;
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
        // Add the straight terminal
        HyperEdge * edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
        score = GetEdgeScore(model, features, sent, *edge);
        Hypothesis * left = left_stack->GetHypothesis(0);
        Hypothesis * right = right_stack->GetHypothesis(0);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, *left, *right);
    	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
    	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
                         left->GetTrgLeft(),
                         right->GetTrgRight(),
                         0, 0, left_stack, right_stack));
        // Add the inverted terminal
        edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
        score = GetEdgeScore(model, features, sent, *edge);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
    	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
    	q.push(new Hypothesis(viterbi_score, score, non_local_score, edge,
						 right->GetTrgLeft(),
						 left->GetTrgRight(),
                         0, 0, left_stack, right_stack));

    }
    // Get a map to store identical target spans
    TargetSpan * ret = new TargetSpan(l, r);
    // Start beam search
    int num_processed = 0;
    while((!beam_size || num_processed < beam_size) && q.size()) {
        // Pop a hypothesis from the stack and get its target span
        Hypothesis * hyp = q.top(); q.pop();
        // Insert the hypothesis
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
        if(hyp->GetCenter() == -1) continue;
        // Increment the left side if there is still a hypothesis left
        left_stack = GetStack(l, hyp->GetCenter()-1);
        new_left_hyp = left_stack->GetHypothesis(hyp->GetLeftRank()+1);
        if(new_left_hyp) {
            old_left_hyp = hyp->GetLeftHyp();
            old_right_hyp = hyp->GetRightHyp();
            Hypothesis *  new_hyp = new Hypothesis(*hyp);
            new_hyp->SetEdge(new HyperEdge(*hyp->GetEdge()));
    		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
    			non_local_score = GetNonLocalScore(model, non_local_features, sent,
    											*new_left_hyp, *old_right_hyp);
    		else
    			non_local_score = GetNonLocalScore(model, non_local_features, sent,
    											*old_right_hyp, *new_left_hyp);
    		new_hyp->SetScore(hyp->GetScore()
    				- old_left_hyp->GetScore() + new_left_hyp->GetScore()
    				- hyp->GetNonLocalScore() + non_local_score);
    		new_hyp->SetNonLocalScore(non_local_score);
            new_hyp->SetLeftRank(hyp->GetLeftRank()+1);
            new_hyp->SetLeftChild(left_stack);
            if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR) {
                new_hyp->SetTrgLeft(new_left_hyp->GetTrgLeft());
            } else {
                new_hyp->SetTrgRight(new_left_hyp->GetTrgRight());
            }
            q.push(new_hyp);
        }
        // Increment the right side if there is still a hypothesis right
        right_stack = GetStack(hyp->GetCenter(),r);
        new_right_hyp = right_stack->GetHypothesis(hyp->GetRightRank()+1);
        if(new_right_hyp) {
            old_left_hyp = hyp->GetLeftHyp();
            old_right_hyp = hyp->GetRightHyp();
            Hypothesis *  new_hyp = new Hypothesis(*hyp);
            new_hyp->SetEdge(new HyperEdge(*hyp->GetEdge()));
    		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
    			non_local_score = GetNonLocalScore(model, non_local_features, sent,
    											*old_left_hyp, *new_right_hyp);
    		else
    			non_local_score = GetNonLocalScore(model, non_local_features, sent,
    											*new_right_hyp, *old_left_hyp);
    		new_hyp->SetScore(hyp->GetScore()
    				- old_right_hyp->GetScore() + new_right_hyp->GetScore()
    				- hyp->GetNonLocalScore() + non_local_score);
    		new_hyp->SetNonLocalScore(non_local_score);
            new_hyp->SetRightRank(hyp->GetRightRank()+1);
            new_hyp->SetRightChild(right_stack);
            if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR) {
                new_hyp->SetTrgRight(new_right_hyp->GetTrgRight());
            } else {
                new_hyp->SetTrgLeft(new_right_hyp->GetTrgLeft());
            }
            q.push(new_hyp);
        }
    }
    while(q.size()) {
    	delete q.top();
    	q.pop();
    }
    sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
    return ret;
}

// Build a hypergraph using beam search and cube pruning
void HyperGraph::BuildHyperGraph(ReordererModel & model,
                                 const FeatureSet & features,
                                 const FeatureSet & non_local_features,
                                 const Sentence & sent,
                                 int beam_size) {
    n_ = sent[0]->GetNumWords();
    stacks_.resize(n_ * (n_+1) / 2 + 1, NULL); // resize stacks in advance
    // Iterate through the left side of the span
    for(int L = 1; L <= n_; L++) {
        // Move the span from l to r, building hypotheses from small to large
        for(int l = 0; l <= n_-L; l++){
            int r = l+L-1;
            SetStack(l, r, ProcessOneSpan(model, features, non_local_features, sent,
                                          l, r, beam_size));
        }
    }
    // Build the root node
    TargetSpan * top = GetStack(0,n_-1);
    TargetSpan * root_stack = new TargetSpan(0,n_);
    for(int i = 0; i < (int)top->size(); i++)
        root_stack->AddHypothesis(new Hypothesis((*top)[i]->GetScore(), 0, 0, 0, n_-1, 0, n_-1,
                HyperEdge::EDGE_ROOT, -1, i, -1, top));
    stacks_[n_ * (n_+1) / 2] = root_stack;
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
