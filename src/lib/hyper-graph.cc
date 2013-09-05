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
#include "lm/word_index.hh"

using namespace lader;
using namespace std;
using namespace boost;
using namespace lm::ngram;

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
	// in order to avoid too much memory usage
	// TODO: make this optional?
    const FeatureVectorInt * fvi =
    		feature_gen.MakeNonLocalFeatures(sent, left, right, model.GetFeatureIds(), model.GetAdd());
    double score = model.ScoreFeatureVector(SafeReference(fvi));
    delete fvi;
    return score;
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
		const FeatureVectorInt * fvi;
		if (hyp.GetEdgeType() == HyperEdge::EDGE_STR)
			fvi = feature_gen.MakeNonLocalFeatures(sent, *left, *right, model.GetFeatureIds(), model.GetAdd());
		else if (hyp.GetEdgeType() == HyperEdge::EDGE_INV)
			fvi = feature_gen.MakeNonLocalFeatures(sent, *right, *left, model.GetFeatureIds(), model.GetAdd());
        BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
            feat_map[feat_pair.first] += feat_pair.second;
        delete fvi;
	}
	if (left)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *left);
	if (right)
		AccumulateNonLocalFeatures(feat_map, model, feature_gen, sent, *right);
}

// If the length is OK, add a terminal
void HyperGraph::AddTerminals(int l, int r, const FeatureSet & features, ReordererModel & model, const Sentence & sent, HypothesisQueue *& q)
{
    // If the length is OK, add a terminal
    if((features.GetMaxTerm() == 0) || (r - l < features.GetMaxTerm())){
        double score, non_local_score;
        // Create a hypothesis with the forward terminal
        Model::State out;
        non_local_score = 0.0;
        if(bigram_){
            Model::State state = bigram_->NullContextState();
            for(int i = l;i <= r;i++){
            	if (i-l+1 < bigram_->Order())
            		bigram_->Score(state, bigram_->GetVocabulary().Index(sent[0]->GetElement(i)), out);
            	else
            		non_local_score +=
					bigram_->Score(state, bigram_->GetVocabulary().Index(sent[0]->GetElement(i)), out);
                state = out;
            }
        }
        HyperEdge *edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_FOR);
        score = GetEdgeScore(model, features, sent, *edge);
        q->push(new Hypothesis(score + non_local_score, score, non_local_score, edge, l, r, out));
        if(features.GetUseReverse()){
            // Create a hypothesis with the backward terminal
            Model::State out;
            non_local_score = 0.0;
            if(bigram_){
                Model::State state = bigram_->NullContextState();
                for(int i = r;i >= l;i--){
                	if (r-i+1 < bigram_->Order())
						bigram_->Score(state, bigram_->GetVocabulary().Index(sent[0]->GetElement(i)), out);
					else
						non_local_score +=
						bigram_->Score(state, bigram_->GetVocabulary().Index(sent[0]->GetElement(i)), out);
                    state = out;
                }
            }
            edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_BAC);
            score = GetEdgeScore(model, features, sent, *edge);
            q->push(new Hypothesis(score + non_local_score, score, non_local_score, edge, r, l, out));
        }

    }

}

// Build a hypergraph using beam search and cube pruning
TargetSpan * HyperGraph::ProcessOneSpan(ReordererModel & model,
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

    else
        // Create the temporary data members for this span
        q = new HypothesisQueue;

    double score, viterbi_score, non_local_score;
    // If the length is OK, add a terminal
    AddTerminals(l, r, features, model, sent, q);

    TargetSpan *left_stack, *right_stack;
    Hypothesis *new_left_hyp, *old_left_hyp,
               *new_right_hyp, *old_right_hyp;
    lm::ngram::Model::State out;
    // Add the best hypotheses for each non-terminal to the queue
    for(int c = l+1; c <= r; c++) {
        // Find the best hypotheses on the left and right side
        left_stack = GetStack(l, c-1);
        right_stack = GetStack(c, r);
        if(left_stack == NULL) THROW_ERROR("Target l="<<l<<", c-1="<<c-1);
        if(right_stack == NULL) THROW_ERROR("Target c="<<c<<", r="<<r);
        Hypothesis * left, *right;
        if (cube_growing_){
        	left = left_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
        	right = right_stack->LazyKthBest(0, model, features, non_local_features, sent, bigram_);
        }
        else{
        	left = left_stack->GetHypothesis(0);
        	right = right_stack->GetHypothesis(0);
        }
        // Add the straight terminal
        HyperEdge * edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
        score = GetEdgeScore(model, features, sent, *edge);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, *left, *right);
    	if (bigram_)
    		non_local_score += bigram_->Score(
					left->GetState(),
					bigram_->GetVocabulary().Index(
							sent[0]->GetElement(right->GetTrgLeft())),
					out);
    	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
    	q->push(new Hypothesis(viterbi_score, score, non_local_score, edge,
                         left->GetTrgLeft(),
                         right->GetTrgRight(),
                         l+1 == r ? out : right->GetState(),
                         0, 0, left_stack, right_stack));
        // Add the inverted terminal
        edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
        score = GetEdgeScore(model, features, sent, *edge);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, *right, *left);
    	if (bigram_)
			non_local_score += bigram_->Score(
					right->GetState(),
					bigram_->GetVocabulary().Index(
							sent[0]->GetElement(left->GetTrgLeft())),
					out);
    	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
    	q->push(new Hypothesis(viterbi_score, score, non_local_score, edge,
						 right->GetTrgLeft(),
						 left->GetTrgRight(),
						 l+1 == r ? out : left->GetState(),
                         0, 0, left_stack, right_stack));

    }
    // For cube growing, search is lazy
    if (cube_growing_)
    	return ret;

    // Start beam search
    int num_processed = 0;
    while((!beam_size || num_processed < beam_size) && q->size()) {
        // Pop a hypothesis from the stack and get its target span
        Hypothesis * hyp = q->top(); q->pop();
        // Insert the hypothesis
        ret->AddHypothesis(hyp);
        num_processed++;
        // If the next hypothesis on the stack is equal to the current
        // hypothesis, remove it, as this just means that we added the same
        // hypothesis
        while(q->size() && *q->top() == *hyp) {
        	delete q->top();
        	q->pop();
        }
        // Skip terminals
        if(hyp->GetCenter() == -1) continue;

    	Hypothesis * new_left = NULL, *new_right = NULL;

    	TargetSpan *left_span = hyp->GetLeftChild();
    	if (left_span)
    		new_left = left_span->GetHypothesis(hyp->GetLeftRank()+1);
    	hyp->IncrementLeft(new_left, model, non_local_features, sent, bigram_, *q);

    	TargetSpan *right_span = hyp->GetRightChild();
    	if (right_span)
    		new_right = right_span->GetHypothesis(hyp->GetRightRank()+1);
    	hyp->IncrementRight(new_right, model, non_local_features, sent, bigram_, *q);
    }
    while(q->size()) {
    	delete q->top();
    	q->pop();
    }
    sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
    delete q;
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
            // TODO: parallelize processing in a row
            // need to lock GetEdgeScore() -> GetEdgeFeatures() -> model.GetFeatureId().GetId()
            // need to lock GetNonLocalScore() -> model.GetFeatureId().GetId()
            // need to lock LazyNext() for cube growing
            SetStack(l, r, ProcessOneSpan(model, features, non_local_features, sent,
                                          l, r, beam_size));
        }
    }
    // Build the root node
    TargetSpan * top = GetStack(0,n_-1);
    TargetSpan * root_stack = new TargetSpan(0,n_);
    for(int i = 0; !beam_size || i < beam_size; i++){
    	Hypothesis * hyp = NULL;
    	if (cube_growing_)
    		hyp = top->LazyKthBest(i, model, features, non_local_features, sent, bigram_);
    	else if (i < (int) top->size())
    		hyp = (*top)[i];
    	if (!hyp)
    		break;
    	double non_local_score = 0.0;
    	if (bigram_){
    		lm::ngram::Model::State state = bigram_->BeginSentenceState();
    		lm::ngram::Model::State out;
    		// begin of sentence
    		non_local_score += bigram_->Score(
					state,
					bigram_->GetVocabulary().Index(
							sent[0]->GetElement(hyp->GetTrgLeft())),
					out);
    		// end of sentence
    		non_local_score += bigram_->Score(
    							hyp->GetState(),
    							bigram_->GetVocabulary().Index("</s>"),
    							out);
    	}
        root_stack->AddHypothesis(new Hypothesis(
        		hyp->GetScore() + non_local_score, 0, non_local_score,
        		0, n_-1,
        		hyp->GetTrgLeft(), hyp->GetTrgRight(),
                HyperEdge::EDGE_ROOT, -1,
                i, -1,
                top, NULL));
    }
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

void HyperGraph::GetReordering(std::vector<int> & reord, Hypothesis * hyp) const{
	if (hyp == NULL)
		return;
    HyperEdge::Type type = hyp->GetEdgeType();
    if(type == HyperEdge::EDGE_FOR) {
        for(int i = hyp->GetLeft(); i <= hyp->GetRight(); i++)
            reord.push_back(i);
    } else if(type == HyperEdge::EDGE_BAC) {
        for(int i = hyp->GetRight(); i >= hyp->GetLeft(); i--)
            reord.push_back(i);
    } else if(type == HyperEdge::EDGE_ROOT) {
        GetReordering(reord, hyp->GetLeftHyp());
    } else if(type == HyperEdge::EDGE_STR) {
        GetReordering(reord, hyp->GetLeftHyp());
        GetReordering(reord, hyp->GetRightHyp());
    } else if(type == HyperEdge::EDGE_INV) {
        GetReordering(reord, hyp->GetRightHyp());
        GetReordering(reord, hyp->GetLeftHyp());
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
