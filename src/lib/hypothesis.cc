/*
 * hypothesis.cc
 *
 *  Created on: May 2, 2013
 *      Author: leona
 */

#include <hypothesis.h>
#include <target-span.h>
#include <discontinuous-target-span.h>
#include <sstream>

using namespace std;
using namespace lader;

inline string GetTokenWord(const string & str) {
    ostringstream oss;
    for(int i = 0; i < (int)str.length(); i++) {
        switch (str[i]) {
            case '(': oss << "-LRB-"; break;
            case ')': oss << "-RRB-"; break;
            case '[': oss << "-LSB-"; break;
            case ']': oss << "-RSB-"; break;
            case '{': oss << "-LCB-"; break;
            case '}': oss << "-RCB-"; break;
            default: oss << str[i];
        }
    }
    return oss.str();
}

void Hypothesis::PrintChildren( std::ostream& out ) const{
	if (left_child_ == NULL && right_child_ == NULL)
		return;
	if (left_child_ && left_rank_ < left_child_->GetHypotheses().size()){
		out << endl << "\t" << left_rank_ << "th LChild ";
		DiscontinuousHypothesis * left =
				dynamic_cast<DiscontinuousHypothesis*>(left_child_->GetHypothesis(left_rank_));
		if (left)
			out << *left;
		else
			out << *left_child_->GetHypothesis(left_rank_);
	}
	if (right_child_ && right_rank_ < right_child_->GetHypotheses().size()){
		out << endl << "\t" << right_rank_ << "th RChild ";
		DiscontinuousHypothesis * right =
				dynamic_cast<DiscontinuousHypothesis*>(right_child_->GetHypothesis(right_rank_));
		if (right)
			out << *right;
		else
			out << *right_child_->GetHypothesis(right_rank_);
	}
	out << endl;
}

void Hypothesis::PrintParse(const vector<string> & strs, ostream & out) const {
    HyperEdge::Type type = GetEdgeType();
    if(IsTerminal()) {
        out << "(" << (char)type;
        for(int i = GetLeft(); i <= GetRight(); i++)
            out << " ("<<(char)type<< "W " << GetTokenWord(strs[i]) <<")";
        out << ")";
    } else if(type == HyperEdge::EDGE_ROOT) {
        GetLeftHyp()->PrintParse(strs, out);
    } else {
        out << "("<<(char)type<<" ";
        GetLeftHyp()->PrintParse(strs, out);
        out << " ";
        GetRightHyp()->PrintParse(strs, out);
        out << ")";
    }
}

Hypothesis * Hypothesis::GetLeftHyp() const{
	if (!left_child_ || left_rank_ >= left_child_->GetHypotheses().size())
		return NULL;
	return left_child_->GetHypothesis(left_rank_);
}

Hypothesis * Hypothesis::GetRightHyp() const{
	if (!right_child_ || right_rank_ >= right_child_->GetHypotheses().size())
		return NULL;
	return right_child_->GetHypothesis(right_rank_);
}

void Hypothesis::GetReordering(std::vector<int> & reord, bool verbose) const{
	HyperEdge::Type type = this->GetEdgeType();
	if (verbose && !this->IsTerminal()){
		cerr << "Loss:" << this->GetLoss();
		const DiscontinuousHypothesis* dhyp =
				dynamic_cast<const DiscontinuousHypothesis*>(this);
		if (dhyp)
			cerr << *dhyp;
		else
			cerr << *this;
		this->PrintChildren(cerr);
	}
	if(type == HyperEdge::EDGE_FOR) {
		for(int i = this->GetLeft(); i <= this->GetRight(); i++)
			reord.push_back(i);
	} else if(type == HyperEdge::EDGE_BAC) {
		for(int i = this->GetRight(); i >= this->GetLeft(); i--)
			reord.push_back(i);
	} else if(type == HyperEdge::EDGE_ROOT) {
		GetLeftHyp()->GetReordering(reord);
	} else if(type == HyperEdge::EDGE_STR) {
		GetLeftHyp()->GetReordering(reord);
		GetRightHyp()->GetReordering(reord);
	} else if(type == HyperEdge::EDGE_INV) {
		GetRightHyp()->GetReordering(reord);
		GetLeftHyp()->GetReordering(reord);
	}
}
// Add up the loss over an entire subtree defined by span of this hypothesis
double Hypothesis::AccumulateLoss() {
    double score = GetLoss();
    if(GetLeftHyp())  score += GetLeftHyp()->AccumulateLoss();
    if(GetRightHyp())  score += GetRightHyp()->AccumulateLoss();
    return score;
}

FeatureVectorInt Hypothesis::AccumulateFeatures(
		const EdgeFeatureMap * features) {
	std::tr1::unordered_map<int,double> feat_map;
	return AccumulateFeatures(feat_map, features);
}

FeatureVectorInt Hypothesis::AccumulateFeatures(
		std::tr1::unordered_map<int,double> & feat_map,
		const EdgeFeatureMap * features) {
    AccumulateFeatures(features, feat_map);
    FeatureVectorInt ret;
    BOOST_FOREACH(FeaturePairInt feat_pair, feat_map)
        ret.push_back(feat_pair);
    return ret;
}

void Hypothesis::AccumulateFeatures(const EdgeFeatureMap * features,
                        std::tr1::unordered_map<int,double> & feat_map) {
    // Find the features
    if(GetEdgeType() != HyperEdge::EDGE_ROOT) {
        EdgeFeatureMap::const_iterator fit =
                                    features->find(GetEdge());
        if(fit == features->end())
            THROW_ERROR("No features found in Accumulate for " << *GetEdge());
        BOOST_FOREACH(FeaturePairInt feat_pair, *(fit->second))
            feat_map[feat_pair.first] += feat_pair.second;
    }
    if(GetLeftHyp()) GetLeftHyp()->AccumulateFeatures(features, feat_map);
    if(GetRightHyp())GetRightHyp()->AccumulateFeatures(features,feat_map);
}

// Get the score for the left and right children of a hypothesis
inline double ComputeNonLocalScore(ReordererModel & model,
                                const FeatureSet & feature_gen,
                                const Sentence & sent,
                                const Hypothesis & left,
                                const Hypothesis & right) {
    const FeatureVectorInt * fvi =
    		feature_gen.MakeNonLocalFeatures(sent, left, right, model.GetFeatureIds(), model.GetAdd());
    double score = model.ScoreFeatureVector(SafeReference(fvi));
    delete fvi;
    return score;
}

// Clone a hypothesis with a hyper edge, which could be discontinuous
Hypothesis *Hypothesis::Clone() const
{
    Hypothesis *new_hyp;
    HyperEdge *edge = this->GetEdge();
    DiscontinuousHyperEdge *dedge = dynamic_cast<DiscontinuousHyperEdge*>(edge);
    // discontinuous + discontinuous = continuous
    if(dedge){
        new_hyp = new Hypothesis(*this);
        new_hyp->SetEdge(dedge->Clone());
    }
    // continuous + continuous = continuous
    else
    {
        new_hyp = new Hypothesis(*this);
        new_hyp->SetEdge(edge->Clone());
    }
    return new_hyp;
}

// We can skip this hypothesis in search
// if this hypothesis produce an ITG permutation from discontinuous children
bool Hypothesis::CanSkip() {
	bool skip = false;
	if (!IsTerminal()) {
		DiscontinuousHypothesis * left =
				dynamic_cast<DiscontinuousHypothesis*>(GetLeftHyp());
		DiscontinuousHypothesis * right =
				dynamic_cast<DiscontinuousHypothesis*>(GetRightHyp());
		// discontinuous + discotinuous = continuous
		return left && right
				&& (GetEdgeType() == left->GetEdgeType()
				|| GetEdgeType() == right->GetEdgeType());
	}
	return false;
}

// Increment the left side if there is still a hypothesis left
void Hypothesis::IncrementLeft(Hypothesis *new_left, ReordererModel & model,
		const FeatureSet & non_local_features, const Sentence & sent,
		const lm::ngram::Model * bigram, HypothesisQueue & q)
{
   if (new_left != NULL){
		Hypothesis * old_left= GetLeftHyp();
		Hypothesis * old_right = GetRightHyp();
        // Clone this hypothesis
		Hypothesis * new_hyp = Clone();
        // Recompute non-local score
		double non_local_score = 0.0;
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*new_left, *old_right);
		else
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*old_right, *new_left);
		if (bigram){
			lm::ngram::Model::State out;
			non_local_score += bigram->Score(
					old_left->GetState(),
					bigram->GetVocabulary().Index(
							sent[0]->GetElement(new_left->GetTrgLeft())),
					GetLeft() +1 != GetRight() ? out : new_hyp->state_);
		}
		new_hyp->SetScore(GetScore()
				- old_left->GetScore() + new_left->GetScore()
				- GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetLeftRank(GetLeftRank() + 1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgLeft(new_left->GetTrgLeft());
		else
			new_hyp->SetTrgRight(new_left->GetTrgRight());
		q.push(new_hyp);
	}
}

// Increment the right side if there is still a hypothesis right
void Hypothesis::IncrementRight(Hypothesis *new_right, ReordererModel & model,
		const FeatureSet & non_local_features, const Sentence & sent,
		const lm::ngram::Model * bigram, HypothesisQueue & q)
{
    if (new_right != NULL){
    	Hypothesis * old_left = GetLeftHyp();
    	Hypothesis * old_right = GetRightHyp();
        // Clone this hypothesis
        Hypothesis * new_hyp = Clone();
        // Recompute non-local score
		double non_local_score = 0.0;
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*old_left, *new_right);
		else
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*new_right, *old_left);
		if (bigram){
			lm::ngram::Model::State out;
			non_local_score += bigram->Score(
					old_left->GetState(),
					bigram->GetVocabulary().Index(
							sent[0]->GetElement(new_right->GetTrgLeft())),
					GetLeft() +1 != GetRight() ? out : new_hyp->state_);
		}
		new_hyp->SetScore(GetScore()
				- old_right->GetScore() + new_right->GetScore()
				- GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetRightRank(GetRightRank()+1);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgRight(new_right->GetTrgRight());
		else
			new_hyp->SetTrgLeft(new_right->GetTrgLeft());
		q.push(new_hyp);
	}
}

// For cube growing
void Hypothesis::LazyNext(HypothesisQueue & q, ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent, const lm::ngram::Model * bigram){
	Hypothesis * new_left = NULL, *new_right = NULL;

	TargetSpan *left_span = GetLeftChild();
	if (left_span)
		new_left = left_span->LazyKthBest(GetLeftRank()+1, model, features, non_local_features, sent, bigram);
    IncrementLeft(new_left, model, non_local_features, sent, bigram, q);

	TargetSpan *right_span = GetRightChild();
	if (right_span)
		new_right = right_span->LazyKthBest(GetRightRank()+1, model, features, non_local_features, sent, bigram);
    IncrementRight(new_right, model, non_local_features, sent, bigram, q);
}
