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
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestCalculateSentenceLoss()" << endl; if(TestCalculateSentenceLoss()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestLossChunk Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    LossLevenshtein lf;

};

}



#endif /* TEST_LOSS_LEVENSHTEIN_H_ */
