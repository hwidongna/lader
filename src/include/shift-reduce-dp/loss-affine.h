/*
 * loss-affine.h
 *
 *  Created on: Apr 4, 2014
 *      Author: leona
 */

#ifndef LOSS_AFFINE_H_
#define LOSS_AFFINE_H_

#include <lader/loss-base.h>

namespace lader {

class LossAffine : public LossBase {
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
    	std::vector< std::vector<double> > M(order.size()+1);
    	std::vector< std::vector<double> > I(order.size()+1);
    	std::vector< std::vector<double> > D(order.size()+1);
    	for (int row = 0 ; row <= order.size(); row++){
    		M[row].resize(gold.size()+1, 0);
    		I[row].resize(gold.size()+1, 0);
    		D[row].resize(gold.size()+1, 0);
    	}
    	M[0][0] = I[0][0] = D[0][0] = 0;
    	for (int row = 0 ; row <= order.size(); row++){
    		for (int col = 0 ; col <= gold.size() ; col++){
    			double ins = 0, del = 0, sub = 0;
    			if (col > 0)
    				ins = I[row][col-1];
    			if (row > 0)
    				del = D[row-1][col];
    			if (row > 0 && col > 0)
    				sub = M[row-1][col-1] + (order[row-1] == gold[col-1] ? 0 : mcost_);
    			M[row][col] = std::min(std::min(ins, del), sub);
				if (row > 0)
					D[row][col] = std::min(M[row-1][col] + scost_, D[row-1][col] + ecost_) + dcost_;
    			if (col > 0)
    				I[row][col] = std::min(M[row][col-1] + scost_, I[row][col-1] + ecost_) + icost_;
    		}
    	}
    	std::pair<double,double> ret(0,0);
    	ret.first = min(min(M[order.size()][gold.size()], I[order.size()][gold.size()]), D[order.size()][gold.size()]);
    	ret.second = std::max(order.size() + 1, gold.size() + 1);
    	ret.first *= weight_;
    	ret.second *= weight_;
    	return ret;
    }
    // Initializes the loss calculator with a ranks
	virtual void Initialize(
			const Ranks * ranks, const FeatureDataParse * parse) {
		icost_ = 3, dcost_ = 3, mcost_ = 3;
		scost_ = 3, ecost_ = 1;
	}
    virtual std::string GetName() const { return "affine"; }
private:
    double icost_, dcost_, mcost_;
    double scost_, ecost_;
};

}

#endif /* LOSS_AFFINE_H_ */
