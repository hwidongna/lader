/*
 * test-loss-edit-distance.h
 *
 *  Created on: Apr 2, 2014
 *      Author: leona
 */

#ifndef TEST_LOSS_LEVENSHTEIN_H_
#define TEST_LOSS_LEVENSHTEIN_H_

#include "test-base.h"
#include <shift-reduce-dp/loss-levenshtein.h>

namespace lader {

class TestLossLevenshtein : public TestBase {

public:

    TestLossLevenshtein() {
    }
    ~TestLossLevenshtein() { }

    int TestCalculateSentenceLoss() {
        int ret = 1;
        Ranks ranks = Ranks::FromString("1 -1 5 4 3 2 6");
        pair<double,double> loss = lf.CalculateSentenceLoss(ranks.GetRanks(), &ranks, NULL);
        if (loss.first != 0){
        	cerr << loss.first << " != 0" << endl;;
        	ret = 0;
        }
        // insert one
        Ranks ranks2 = Ranks::FromString("0 1 -1 5 4 3 2 6");
        loss = lf.CalculateSentenceLoss(ranks2.GetRanks(), &ranks, NULL);
        if (loss.first != 1){
			cerr << loss.first << " != 1" << endl;;
			ret = 0;
		}
        // delete one
        Ranks ranks3 = Ranks::FromString("1 5 4 3 2 6");
        loss = lf.CalculateSentenceLoss(ranks3.GetRanks(), &ranks, NULL);
        if (loss.first != 1){
        	cerr << loss.first << " != 1" << endl;;
        	ret = 0;
        }
        // substitute one
		Ranks ranks4 = Ranks::FromString("0 -1 5 4 3 2 6");
		loss = lf.CalculateSentenceLoss(ranks4.GetRanks(), &ranks, NULL);
		if (loss.first != 1){
			cerr << loss.first << " != 1" << endl;;
			ret = 0;
		}
		// mixed
		Ranks ranks5 = Ranks::FromString("0 -1 4 5 3 2 6");
		loss = lf.CalculateSentenceLoss(ranks5.GetRanks(), &ranks, NULL);
		if (loss.first != 1+1+1){
			cerr << loss.first << " != 3" << endl;;
			ret = 0;
		}

        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestCalculateSentenceLoss()" << endl; if(TestCalculateSentenceLoss()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestLossLevenshtein Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    LossLevenshtein lf;

};

}



#endif /* TEST_LOSS_LEVENSHTEIN_H_ */
