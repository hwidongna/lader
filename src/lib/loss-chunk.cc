#include <lader/loss-chunk.h>
#include <lader/target-span.h>

using namespace lader;
using namespace std;

double LossChunk::AddLossToProduction(Hypothesis * hyp,
        const Ranks * ranks, const FeatureDataParse * parse) {
	int trg_left = hyp->GetTrgLeft(),
		trg_right = hyp->GetTrgRight();
    int trg_midleft, trg_midright;
    if(hyp->GetEdgeType() == HyperEdge::EDGE_STR) {
    	trg_midleft = hyp->GetLeftHyp()->GetTrgRight();
    	trg_midright = hyp->GetRightHyp()->GetTrgLeft();
    } else if(hyp->GetEdgeType() == HyperEdge::EDGE_INV) {
    	trg_midleft = hyp->GetRightHyp()->GetTrgRight();
    	trg_midright = hyp->GetLeftHyp()->GetTrgLeft();
    }
	return AddLossToProduction(hyp->GetLeft(), hyp->GetCenter(), hyp->GetRight(),
			trg_left, trg_midleft, trg_midright, trg_right, hyp->GetEdgeType(), ranks, parse);
}

bool LossChunk::IsStraight(const Ranks * ranks, int trg_midleft, int trg_midright)
{
    return Ranks::IsStepOneUp((*ranks)[trg_midleft], (*ranks)[trg_midright]) ||
    		((*ranks)[trg_midleft] == (*ranks)[trg_midright] && trg_midleft + 1 == trg_midright);
}

double LossChunk::AddLossToProduction(
        int src_left, int src_mid, int src_right,
        int trg_left, int trg_midleft, int trg_midright, int trg_right,
        HyperEdge::Type type,
        const Ranks * ranks, const FeatureDataParse * parse) {
    if(!ranks) THROW_ERROR("Chunk loss requires alignment input");
    int loss = 0;
    switch (type) {
        // For roots, check that both edges are contiguous
        case HyperEdge::EDGE_ROOT:
            if(!Ranks::IsContiguous(-1, (*ranks)[trg_left]))
                loss++;
            if(!Ranks::IsContiguous((*ranks)[trg_right], (*ranks).GetMaxRank()+1))
                loss++;
            break;
        // For straight and inverse non-terms, check inside values
		// If inside rank values are same,
		// they should be straight. If not, it is a loss.
        case HyperEdge::EDGE_STR:
        case HyperEdge::EDGE_INV:
        	if(!IsStraight(ranks, trg_midleft, trg_midright))
				loss++;
			break;
        case HyperEdge::EDGE_FOR:
            for(int i = trg_left; i < trg_right; i++)
                if(!IsStraight(ranks, i, i+1))
                    loss++;
            break;
        case HyperEdge::EDGE_BAC:
            for(int i = trg_left; i > trg_right; i--)
                if(!Ranks::IsStepOneUp((*ranks)[i], (*ranks)[i-1]))
                    loss++;
            break;
        default:
            THROW_ERROR("Bad edge type in LossChunk");
    }
    return loss*weight_;
}


// Calculate the accuracy of a single sentence
std::pair<double,double> LossChunk::CalculateSentenceLoss(
        const std::vector<int> order,
        const Ranks * ranks, const FeatureDataParse * parse) {
    if(!ranks) THROW_ERROR("Chunk loss requires alignment input");
    std::pair<double,double> ret(0,0);
    int n = order.size();
    ret.second = n + 1;
    // Calculate the boundaries
    if(!Ranks::IsContiguous(-1, (*ranks)[order[0]])) {
        ret.first++;
    }
    if(!Ranks::IsContiguous((*ranks)[order[n-1]], (*ranks).GetMaxRank()+1)) {
        ret.first++;
    }
    // Calculate all the rest
    for(int i = 1; i < n; i++)
    	if(!IsStraight(ranks, order[i-1], order[i]))
            ret.first++;
    ret.first *= weight_;
    ret.second *= weight_;
    return ret;
}
