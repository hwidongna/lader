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
#include <lader/thread-pool.h>
#include <boost/thread/mutex.hpp>

using namespace lader;
using namespace std;
using namespace boost;
using namespace lm::ngram;

// Return the edge feature vector
// this is called only once from HyperGraph::GetEdgeScore
// therefore, we don't need to insert and find features in EdgeFeatureMap
// unless we use -save_features option
const FeatureVectorInt * HyperGraph::GetEdgeFeatures(
                                ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge & edge,
                                bool insert) {
	FeatureVectorInt * ret;
    if(features_ && insert) {
        ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
        {
        	boost::mutex::scoped_lock lock(mutex_);
        	features_->insert(MakePair(edge.Clone(), ret));
        }
    } else if (features_) {
    	EdgeFeatureMap::const_iterator it = features_->find(&edge);
        ret = it->second;
    } else {
    	// no insert, make -threads faster without -save_features
    	ret = feature_gen.MakeEdgeFeatures(sent, edge, model.GetFeatureIds(), model.GetAdd());
    }
    return ret;
}

const FeatureVectorInt *HyperGraph::GetNonLocalFeatures(const HyperEdge * edge,
		const Hypothesis *left, const Hypothesis *right, const FeatureSet & feature_gen,
		const Sentence & sent, ReordererModel & model, lm::ngram::State * out)
{
	if (left && right && left->GetLeft() > right->GetLeft())
		THROW_ERROR("Invalid argument for GetNonLocalFeatures" << endl << *left << endl << *right)
    FeatureVectorInt *fvi = NULL;
    SymbolSet<int> & feat_ids = model.GetFeatureIds();
    bool add = model.GetAdd();
    FeaturePairInt bigram(feat_ids.GetId("BIGRAM", add), 0.0);
    // use reordered bigram as a non-local feature
    // assume sent[0] is lexical information
    // TODO: bigram of POS tags, word classes
    if (edge->GetType() == HyperEdge::EDGE_STR){
		fvi = feature_gen.MakeNonLocalFeatures(sent, *left, *right, feat_ids, add);
		if (bigram_ && out)
			bigram.second = bigram_->Score(
					left->GetState(),
					bigram_->GetVocabulary().Index(
							sent[0]->GetElement(right->GetTrgLeft())), *out);
    }
	else if (edge->GetType() == HyperEdge::EDGE_INV){
		fvi = feature_gen.MakeNonLocalFeatures(sent, *right, *left, feat_ids, add);
		if (bigram_ && out)
			bigram.second = bigram_->Score(
					right->GetState(),
					bigram_->GetVocabulary().Index(
							sent[0]->GetElement(left->GetTrgLeft())), *out);
	}
	else if (edge->GetType() == HyperEdge::EDGE_FOR){
		if(bigram_){
			fvi = new FeatureVectorInt;
			Model::State state = bigram_->NullContextState();
			for (int i = edge->GetLeft(); i <= edge->GetRight(); i++) {
				if (i - edge->GetLeft() + 1 < bigram_->Order())
					bigram_->Score(
							state,
							bigram_->GetVocabulary().Index(
									sent[0]->GetElement(i)), *out);
				else
					bigram.second += bigram_->Score(
							state,
							bigram_->GetVocabulary().Index(
									sent[0]->GetElement(i)), *out);
				state = *out;
			}
		}
	}
	else if (edge->GetType() == HyperEdge::EDGE_BAC){
		if(bigram_){
			fvi = new FeatureVectorInt;
			Model::State state = bigram_->NullContextState();
			for (int i = edge->GetRight(); i >= edge->GetLeft(); i--) {
				if (edge->GetRight() - i + 1 < bigram_->Order())
					bigram_->Score(
							state,
							bigram_->GetVocabulary().Index(
									sent[0]->GetElement(i)), *out);
				else
					bigram.second += bigram_->Score(
							state,
							bigram_->GetVocabulary().Index(
									sent[0]->GetElement(i)), *out);
				state = *out;
			}
		}
	}
	else if (edge->GetType() == HyperEdge::EDGE_ROOT)
		THROW_ERROR("Use RootBigram instead of GetNonLocalFeatures")
    if (fvi && bigram_ && out)
    	fvi->push_back(bigram);
    return fvi;
}

double HyperGraph::Score(double loss_multiplier,
                         const Hypothesis* hyp) const{
    double score = hyp->GetLoss()*loss_multiplier;
	score += hyp->GetSingleScore() + hyp->GetNonLocalScore();
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
                GetEdgeFeatures(model, feature_gen, sent, edge, true);
    double score = model.ScoreFeatureVector(SafeReference(vec));
    // features are not stored, thus delete
    if (!features_)
    	delete vec;
    return score;
}

// Get the non-local score for a hypothesis to be produced
// If we use the reordered bigram, out stores the LM state
double HyperGraph::GetNonLocalScore(ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const HyperEdge * edge,
                                const Hypothesis * left,
                                const Hypothesis * right,
                                lm::ngram::State * out) {
    // unlike edge features, do not store non-local features
	// in order to avoid too much memory usage
    const FeatureVectorInt * fvi = GetNonLocalFeatures(edge, left, right,
    		feature_gen, sent, model, out);
    if (!fvi)
    	return 0.0;
    double score = model.ScoreFeatureVector(SafeReference(fvi));
    delete fvi;
    return score;
}

// Accumulate edge features under a hyper-edge
// Accumulate non-local features under a hypothesis
void HyperGraph::AccumulateFeatures(std::tr1::unordered_map<int,double> & feat_map,
										ReordererModel & model,
		                                const FeatureSet & features,
		                                const FeatureSet & non_local_features,
		                                const Sentence & sent,
		                                const Hypothesis * hyp){
	Hypothesis * left = hyp->GetLeftHyp();
	Hypothesis * right = hyp->GetRightHyp();
	// root has no edge feature
	// root has only the bigram feature on the boundaries
	if (hyp->GetEdgeType() == HyperEdge::EDGE_ROOT){
		if (bigram_){
			lm::ngram::State out;
			feat_map[model.GetFeatureIds().GetId("BIGRAM")] += GetRootBigram(sent, hyp, &out);
		}
	}
	else{
		// accumulate edge features
		const FeatureVectorInt * fvi = GetEdgeFeatures(model, features, sent, *hyp->GetEdge(), false);
		BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
			feat_map[feat_pair.first] += feat_pair.second;
		// if fvi is not stored, delete
		if (!features_)
			delete fvi;
		// accumulate non-local features
		lm::ngram::State out;
		fvi = GetNonLocalFeatures(hyp->GetEdge(), left, right,
				non_local_features, sent, model, &out);
		if (fvi){
			BOOST_FOREACH(FeaturePairInt feat_pair, *fvi)
            		feat_map[feat_pair.first] += feat_pair.second;
			delete fvi;
		}
	}
	if (left)
		AccumulateFeatures(feat_map, model, features, non_local_features, sent, left);
	if (right)
		AccumulateFeatures(feat_map, model, features, non_local_features, sent, right);
}

// If the length is OK, add a terminal
void HyperGraph::AddTerminals(int l, int r, const FeatureSet & features, ReordererModel & model, const Sentence & sent, HypothesisQueue *& q)
{
    // If the length is OK, add a terminal
    if((features.GetMaxTerm() == 0) || (r - l < features.GetMaxTerm())){
        double score, non_local_score;
        // Create a hypothesis with the forward terminal
        Model::State out;
        HyperEdge *edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_FOR);
        score = GetEdgeScore(model, features, sent, *edge);
        non_local_score = GetNonLocalScore(model, features, sent, edge, NULL, NULL, &out);
        q->push(new Hypothesis(score + non_local_score, score, non_local_score, edge, l, r, out));
        if(features.GetUseReverse()){
            edge = new HyperEdge(l, -1, r, HyperEdge::EDGE_BAC);
            score = GetEdgeScore(model, features, sent, *edge);
            non_local_score = GetNonLocalScore(model, features, sent, edge, NULL, NULL, &out);
            q->push(new Hypothesis(score + non_local_score, score, non_local_score, edge, r, l, out));
        }

    }

}


// Increment the left side if there is still a hypothesis left
void HyperGraph::IncrementLeft(const Hypothesis *old_hyp,
		const Hypothesis *new_left, ReordererModel & model,
		const FeatureSet & non_local_features, const Sentence & sent,
		HypothesisQueue & q)
{
   if (new_left != NULL){
		Hypothesis * old_left= old_hyp->GetLeftHyp();
		Hypothesis * old_right = old_hyp->GetRightHyp();
        // Clone this hypothesis
		Hypothesis * new_hyp = old_hyp->Clone();
        // Recompute non-local score
		lm::ngram::Model::State out;
		double non_local_score = 0.0;
		non_local_score = GetNonLocalScore(model, non_local_features, sent,
				old_hyp->GetEdge(), new_left, old_right, &out);
		if (old_hyp->GetLeft() +1 == old_hyp->GetRight())
			new_hyp->SetState(out);
		new_hyp->SetScore(old_hyp->GetScore()
				- old_left->GetScore() + new_left->GetScore()
				- old_hyp->GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetLeftRank(old_hyp->GetLeftRank()+1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgLeft(new_left->GetTrgLeft());
		else
			new_hyp->SetTrgRight(new_left->GetTrgRight());
		q.push(new_hyp);
	}
}

// Increment the right side if there is still a hypothesis right
void HyperGraph::IncrementRight(const Hypothesis *old_hyp,
		const Hypothesis *new_right, ReordererModel & model,
		const FeatureSet & non_local_features, const Sentence & sent,
		HypothesisQueue & q)
{
    if (new_right != NULL){
    	Hypothesis * old_left = old_hyp->GetLeftHyp();
    	Hypothesis * old_right = old_hyp->GetRightHyp();
        // Clone this hypothesis
        Hypothesis * new_hyp = old_hyp->Clone();
        // Recompute non-local score
        lm::ngram::Model::State out;
		double non_local_score = 0.0;
		non_local_score = GetNonLocalScore(model, non_local_features, sent,
				old_hyp->GetEdge(), old_left, new_right, &out);
		if (old_hyp->GetLeft() +1 == old_hyp->GetRight())
			new_hyp->SetState(out);
		new_hyp->SetScore(old_hyp->GetScore()
				- old_right->GetScore() + new_right->GetScore()
				- old_hyp->GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetRightRank(old_hyp->GetRightRank()+1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgRight(new_right->GetTrgRight());
		else
			new_hyp->SetTrgLeft(new_right->GetTrgLeft());
		q.push(new_hyp);
	}
}

// For cube growing
void HyperGraph::LazyNext(HypothesisQueue & q, ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent, const Hypothesis * hyp){
	Hypothesis * new_left = NULL, *new_right = NULL;

	TargetSpan *left_span = hyp->GetLeftChild();
	if (left_span)
		new_left = LazyKthBest(left_span, hyp->GetLeftRank() + 1,
				model, features, non_local_features, sent);
    IncrementLeft(hyp, new_left, model, non_local_features, sent, q);

	TargetSpan *right_span = hyp->GetRightChild();
	if (right_span)
		new_right = LazyKthBest(right_span, hyp->GetRightRank() + 1,
				model, features, non_local_features, sent);
    IncrementRight(hyp, new_right, model, non_local_features, sent, q);
}

// For cube growing
Hypothesis * HyperGraph::LazyKthBest(TargetSpan * stack, int k,
		ReordererModel & model, const FeatureSet & features,
		const FeatureSet & non_local_features, const Sentence & sent){
	while (stack->size() < k+1 && stack->CandSize() > 0){
		HypothesisQueue & q = stack->GetCands();
		Hypothesis * hyp = q.top(); q.pop();
		LazyNext(q, model, features, non_local_features, sent, hyp);
		// skip unnecessary hypothesis
		// Insert the hypothesis if unique
		if (hyp->CanSkip() || !stack->AddUniqueHypothesis(hyp))
			delete hyp;
	}
	return stack->GetHypothesis(k);

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
        	// instead LazyKthBest, access to the best one in HypothesisQueue
        	// this seems to be risky, but LazyKthBest(0) == HypothesisQueue.top()
        	left = left_stack->GetCands().top();
        	right = right_stack->GetCands().top();
        }
        else{
        	left = left_stack->GetHypothesis(0);
        	right = right_stack->GetHypothesis(0);
        }
        // Add the straight terminal
        HyperEdge * edge = new HyperEdge(l, c, r, HyperEdge::EDGE_STR);
        score = GetEdgeScore(model, features, sent, *edge);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
    	viterbi_score = score + non_local_score + left->GetScore() + right->GetScore();
    	q->push(new Hypothesis(viterbi_score, score, non_local_score, edge,
                         left->GetTrgLeft(),
                         right->GetTrgRight(),
                         l+1 == r ? out : right->GetState(),
                         0, 0, left_stack, right_stack));
        // Add the inverted terminal
        edge = new HyperEdge(l, c, r, HyperEdge::EDGE_INV);
        score = GetEdgeScore(model, features, sent, *edge);
    	non_local_score = GetNonLocalScore(model, non_local_features, sent, edge, left, right, &out);
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
        // Insert the hypothesis if unique
        bool skip = !ret->AddUniqueHypothesis(hyp);
        if (!skip)
        	num_processed++;
        // Skip terminals
        if(hyp->GetCenter() == -1) continue;

    	Hypothesis * new_left = NULL, *new_right = NULL;

    	TargetSpan *left_span = hyp->GetLeftChild();
    	if (left_span)
    		new_left = left_span->GetHypothesis(hyp->GetLeftRank()+1);
    	IncrementLeft(hyp, new_left, model, non_local_features, sent, *q);

    	TargetSpan *right_span = hyp->GetRightChild();
    	if (right_span)
    		new_right = right_span->GetHypothesis(hyp->GetRightRank()+1);
    	IncrementRight(hyp, new_right, model, non_local_features, sent, *q);
        if (skip)
        	delete hyp;
    }
    while(q->size()) {
    	delete q->top();
    	q->pop();
    }
    sort(ret->GetHypotheses().begin(), ret->GetHypotheses().end(), DescendingScore<Hypothesis>());
    delete q;
    return ret;
}

double HyperGraph::GetRootBigram(const Sentence & sent, const Hypothesis *hyp, lm::ngram::Model::State * out)
{
    double non_local_score = 0.0;
    if(bigram_){
        lm::ngram::Model::State state = bigram_->BeginSentenceState();
        // begin of sentence
        non_local_score += bigram_->Score(state,
				bigram_->GetVocabulary().Index(
						sent[0]->GetElement(hyp->GetTrgLeft())), *out);
		// end of sentence
		non_local_score += bigram_->Score(hyp->GetState(),
				bigram_->GetVocabulary().Index("</s>"), *out);
    }
    return non_local_score;
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
    	// parallelize processing in a row
    	ThreadPool pool(threads_, 1000);
    	for(int l = 0; l <= n_-L; l++){
    		int r = l+L-1;
    		SpanTask * task = new SpanTask(this, model, features,
					non_local_features, sent, l, r, beam_size);
    		pool.Submit(task);
    	}
    	pool.Stop(true);
    }
    // Build the root node
    TargetSpan * top = GetStack(0,n_-1);
    TargetSpan * root_stack = new TargetSpan(0,n_);
    for(int i = 0; !beam_size || i < beam_size; i++){
    	Hypothesis * hyp = NULL;
    	if (cube_growing_)
    		hyp = LazyKthBest(top, i, model, features, non_local_features, sent);
    	else if (i < (int)(top->size()))
            hyp = (*top)[i];

		if(!hyp)
			break;

		lm::ngram::State out;
    	double non_local_score = GetRootBigram(sent, hyp, &out) * model.GetWeight("BIGRAM");
        root_stack->AddHypothesis(new Hypothesis(
        		hyp->GetScore() + non_local_score, 0, non_local_score,
        		new HyperEdge(0, -1, n_-1, HyperEdge::EDGE_ROOT),
        		hyp->GetTrgLeft(), hyp->GetTrgRight(),
        		out,
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
