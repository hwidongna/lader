/*
 * loss-edit-distance.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef LOSS_EDIT_DISTANCE_H_
#define LOSS_EDIT_DISTANCE_H_

#include <lader/loss-base.h>

namespace lader {

class LossEditDistance : public LossBase {
public:

	virtual double AddLossToProduction(Hypothesis * hyp,
	    		const Ranks * ranks, const FeatureDataParse * parse) { THROW_ERROR("Unsupported operation"); return 0; }

    virtual double AddLossToProduction(
        int src_left, int src_mid, int src_right,
        int trg_left, int trg_midleft, int trg_midright, int trg_right,
        HyperEdge::Type type,
        const Ranks * ranks, const FeatureDataParse * parse) { THROW_ERROR("Unsupported operation"); return 0; }

    // Calculate the accuracy of a single sentence
    virtual std::pair<double,double> CalculateSentenceLoss(
            const std::vector<int> order,
            const Ranks * ranks, const FeatureDataParse * parse) {
    	const std::vector<int> gold = ranks->GetRanks();
    	// initialize the matrix
    	std::vector< std::vector<double> > score(order.size()+1);
    	for (int row = 0 ; row <= order.size(); row++)
    		score[row].resize(gold.size()+1, 0);
    	score[0][0] = 0;
    	for (int row = 0 ; row <= order.size(); row++){
    		for (int col = 0 ; col <= gold.size() ; col++){
    			double ins = 0, del = 0, sub = 0;
    			if (col > 0)
    				ins = score[row][col-1] + 1;
    			if (row > 0)
    				del = score[row-1][col] + 1;
    			if (row > 0 && col > 0)
    				sub = score[row-1][col-1] + (order[row-1] == gold[col-1] ? 0 : 1);
    			score[row][col] = std::min(std::min(ins, del), sub);
    		}
    	}
    	std::pair<double,double> ret(0,0);
    	ret.first = score[order.size()][gold.size()];
    	ret.second = std::max(order.size() + 1, gold.size() + 1);
    	ret.first *= weight_;
    	ret.second *= weight_;
    	return ret;
    }
//
//    // Initializes the loss calculator with a ranks
//    virtual void Initialize(
//        const Ranks * ranks, const FeatureDataParse * parse);

    virtual std::string GetName() const { return "edit-distance"; }

private:
//
//    // Get the loss of a single span
//    int GetLoss(const Ranks & ranks, int l, int c, int r,
//                    bool straight,std::vector<int> & losses);
//
//    int GetLossStraight(const Ranks & ranks, int l, int c, int r) {
//        return GetLoss(ranks, l, c, r, true, straight_);
//    }
//    int GetLossInverse(const Ranks & ranks, int l, int c, int r) {
//        return GetLoss(ranks, l, c, r, false, inverse_);
//    }
//    int & AccessArray(std::vector<int> & arr, int l, int c, int r, int n) {
//        return SafeAccess(arr, l*n*n+c*n+r);
//    }
//
//    // Get the loss for a particular array
//    std::vector<int> straight_, inverse_;

};

}


#endif /* LOSS_EDIT_DISTANCE_H_ */
