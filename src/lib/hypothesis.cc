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

// For cube growing
void Hypothesis::LazyNext(HypothesisQueue & q, ReordererModel & model,
		const FeatureSet & features, const FeatureSet & non_local_features,
		const Sentence & sent){
	Hypothesis * new_hyp,
	*new_left_hyp = NULL, *old_left_hyp = NULL,
	*new_right_hyp = NULL, *old_right_hyp = NULL;
	double non_local_score;
    HyperEdge *edge = this->GetEdge();
    DiscontinuousHyperEdge *dedge =
    		dynamic_cast<DiscontinuousHyperEdge*>(edge);
	DiscontinuousHypothesis * dhyp =
			dynamic_cast<DiscontinuousHypothesis*>(this);
	TargetSpan *left_span = GetLeftChild();
	// Increment the left side if there is still a hypothesis left
	if (left_span)
		new_left_hyp = left_span->LazyKthBest(GetLeftRank()+1, model, features, non_local_features, sent);
	if (new_left_hyp != NULL){
        old_left_hyp = GetLeftHyp();
        old_right_hyp = GetRightHyp();
        // discontinuous + continuous = discontinuous
        // continuous + discontinuous = discontinuous
        if (dedge && dhyp){
        	new_hyp = new DiscontinuousHypothesis(*dhyp);
        	new_hyp->SetEdge(dedge->Clone());
        }
        // discontinuous + discontinuous = continuous
        else if (dedge && !dhyp){
        	new_hyp = new Hypothesis(*this);
        	new_hyp->SetEdge(dedge->Clone());
        }
        // continuous + continuous = continuous
        else{
        	new_hyp = new Hypothesis(*this);
        	new_hyp->SetEdge(edge->Clone());
        }
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*new_left_hyp, *old_right_hyp);
		else
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*old_right_hyp, *new_left_hyp);
		new_hyp->SetScore(GetScore()
				- old_left_hyp->GetScore() + new_left_hyp->GetScore()
				- GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetLeftRank(GetLeftRank() + 1);
		new_hyp->SetLeftChild(left_span);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgLeft(new_left_hyp->GetTrgLeft());
		else
			new_hyp->SetTrgRight(new_left_hyp->GetTrgRight());
		q.push(new_hyp);
	}
	TargetSpan *right_span = GetRightChild();
	// Increment the right side if there is still a hypothesis right
	if (right_span)
		new_right_hyp = right_span->LazyKthBest(GetRightRank()+1, model, features, non_local_features, sent);
	if (new_right_hyp != NULL){
        old_left_hyp = GetLeftHyp();
        old_right_hyp = GetRightHyp();
        // discontinuous + continuous = discontinuous
        // continuous + discontinuous = discontinuous
        if (dedge && dhyp){
        	new_hyp = new DiscontinuousHypothesis(*dhyp);
        	new_hyp->SetEdge(dedge->Clone());
        }
        // discontinuous + discontinuous = continuous
        else if (dedge && !dhyp){
        	new_hyp = new Hypothesis(*this);
        	new_hyp->SetEdge(dedge->Clone());
        }
        // continuous + continuous = continuous
        else{
        	new_hyp = new Hypothesis(*this);
        	new_hyp->SetEdge(edge->Clone());
        }
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*old_left_hyp, *new_right_hyp);
		else
			non_local_score = ComputeNonLocalScore(model, non_local_features, sent,
											*new_right_hyp, *old_left_hyp);
		new_hyp->SetScore(GetScore()
				- old_right_hyp->GetScore() + new_right_hyp->GetScore()
				- GetNonLocalScore() + non_local_score);
		new_hyp->SetNonLocalScore(non_local_score);
		new_hyp->SetRightRank(GetRightRank()+1);
		new_hyp->SetRightChild(right_span);
		if(new_hyp->GetEdgeType() == HyperEdge::EDGE_STR)
			new_hyp->SetTrgRight(new_right_hyp->GetTrgRight());
		else
			new_hyp->SetTrgLeft(new_right_hyp->GetTrgLeft());
		q.push(new_hyp);
	}
}
