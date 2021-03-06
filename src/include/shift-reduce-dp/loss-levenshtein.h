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

class LossLevenshtein : public LossBase {
public:

	virtual double AddLossToProduction(Hypothesis * hyp,
			const Ranks * ranks, const FeatureDataParse * parse) { THROW_ERROR("Unsupported operation"); return 0; }

	virtual double GetStateLoss(DPState * state, bool root,
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
    	// set max loss to the length of *gold* rank
    	// in order to prevent different max losses between different orders
    	double n = gold.size();
    	for (int row = 0 ; row <= order.size(); row++){
    		for (int col = 0 ; col <= gold.size() ; col++){
    			if (row == 0 && col == 0){
    				score[0][0] = 0;
    				continue;
    			}
    			double ins = n, del = n, sub = n;
    			if (col > 0)
    				ins = score[row][col-1] + 1;
    			if (row > 0)
    				del = score[row-1][col] + 1;
    			if (row > 0 && col > 0)
    				sub = score[row-1][col-1] + (order[row-1] == gold[col-1] ? 0 : 1);
    			// the upper bound of loss is n
    			score[row][col] = std::min(std::min(std::min(ins, del), sub), n);
    		}
    	}
    	std::pair<double,double> ret(0,0);
    	ret.first = score[order.size()][gold.size()];
    	ret.second = n;
    	ret.first *= weight_;
    	ret.second *= weight_;
    	return ret;
    }

    virtual std::string GetName() const { return "levenshtein"; }
};

}


#endif /* LOSS_EDIT_DISTANCE_H_ */
