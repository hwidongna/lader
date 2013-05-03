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

FeatureVectorInt Hypothesis::AccumulateFeatures(const EdgeFeatureMap * features) {
    std::tr1::unordered_map<int,double> feat_map;
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
