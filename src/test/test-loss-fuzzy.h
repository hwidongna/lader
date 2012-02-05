#ifndef TEST_LOSS_FUZZY_H__
#define TEST_LOSS_FUZZY_H__

#include "test-base.h"
#include <kyldr/combined-alignment.h>
#include <kyldr/loss-fuzzy.h>

namespace kyldr {

class TestLossFuzzy : public TestBase {

public:

    TestLossFuzzy() {
        // Create an alignment that looks like this
        // xx..
        // ...x
        // ..x.
        Alignment al(MakePair(4,3));
        al.AddAlignment(MakePair(0,0));
        al.AddAlignment(MakePair(1,0));
        al.AddAlignment(MakePair(2,2));
        al.AddAlignment(MakePair(3,1));
        ranks = Ranks(CombinedAlign(al));
    }
    ~TestLossFuzzy() { }

    int TestStraightNonterminal() {
        int ret = 1;
        // Create an equal node, loss==0
        double loss01 = lf.AddLossToProduction(0,0,1,1,
                                               HyperEdge::EDGE_STR,ranks);
        if(loss01 != 0) {
            cerr << "loss01 "<<loss01<<" != 0"<<endl; ret = 0;
        }
        // Create a skipped node, loss==1
        double loss12 = lf.AddLossToProduction(1,1,2,2,
                                               HyperEdge::EDGE_STR,ranks);
        if(loss12 != 1) {
            cerr << "loss12 "<<loss12<<" != 1"<<endl; ret = 0;
        }
        // Create a reversed node, loss==1
        double loss23 = lf.AddLossToProduction(2,2,3,3,
                                               HyperEdge::EDGE_STR,ranks);
        if(loss23 != 1) {
            cerr << "loss23 "<<loss23<<" != 1"<<endl; ret = 0;
        }
        return ret;
    }

    int TestInvertedNonterminal() {
        int ret = 1;
        // Create an equal node, loss==0
        double loss01 = lf.AddLossToProduction(1,1,0,0,
                                               HyperEdge::EDGE_INV,ranks);
        if(loss01 != 0) {
            cerr << "loss01 "<<loss01<<" != 0"<<endl; ret = 0;
        }
        // Create a skipped node, loss==1
        double loss12 = lf.AddLossToProduction(2,2,1,1,
                                               HyperEdge::EDGE_INV,ranks);
        if(loss12 != 1) {
            cerr << "loss12 "<<loss12<<" != 1"<<endl; ret = 0;
        }
        // Create a reversed node, loss==0
        double loss23 = lf.AddLossToProduction(3,3,2,2,
                                               HyperEdge::EDGE_INV,ranks);
        if(loss23 != 0) {
            cerr << "loss23 "<<loss23<<" != 0"<<endl; ret = 0;
        }
        return ret;
    }

    int TestStraightTerminal() {
        int ret = 1;
        // Create an equal node, loss==0
        double loss01 = lf.AddLossToProduction(0,0,1,1,
                                               HyperEdge::EDGE_FOR,ranks);
        if(loss01 != 0) {
            cerr << "loss01 "<<loss01<<" != 0"<<endl; ret = 0;
        }
        // Create a skipped node, loss==0
        double loss12 = lf.AddLossToProduction(1,1,2,2,
                                               HyperEdge::EDGE_FOR,ranks);
        if(loss12 != 1) {
            cerr << "loss12 "<<loss12<<" != 1"<<endl; ret = 0;
        }
        // Create a three-word, loss==2
        double loss13 = lf.AddLossToProduction(1,1,3,3,
                                               HyperEdge::EDGE_FOR,ranks);
        if(loss13 != 2) {
            cerr << "loss13 "<<loss13<<" != 2"<<endl; ret = 0;
        }
        return ret;
    }

    int TestInvertedTerminal() {
        int ret = 1;
        // Create an equal node, loss==0
        double loss01 = lf.AddLossToProduction(1,-1,-1,0,
                                               HyperEdge::EDGE_BAC,ranks);
        if(loss01 != 0) {
            cerr << "loss01 "<<loss01<<" != 0"<<endl; ret = 0;
        }
        // Create a skipped node, loss==1
        double loss12 = lf.AddLossToProduction(2,-1,-1,1,
                                               HyperEdge::EDGE_BAC,ranks);
        if(loss12 != 1) {
            cerr << "loss12 "<<loss12<<" != 1"<<endl; ret = 0;
        }
        // Create a three-word node, loss==1
        double loss13 = lf.AddLossToProduction(3,-1,-1,1,
                                               HyperEdge::EDGE_BAC,ranks);
        if(loss13 != 1) {
            cerr << "loss13 "<<loss13<<" != 1"<<endl; ret = 0;
        }
        return ret;
    }

    int TestRoot() {
        int ret = 1;
        // Create an equal node, loss==0
        double loss1 = lf.AddLossToProduction(1,-1,-1,2,
                                               HyperEdge::EDGE_ROOT,ranks);
        if(loss1 != 0) {
            cerr << "loss1 "<<loss1<<" != 0"<<endl; ret = 0;
        }
        // Create a reversed node, loss==2
        double loss2 = lf.AddLossToProduction(2,-1,-1,1,
                                              HyperEdge::EDGE_ROOT,ranks);
        if(loss2 != 2) {
            cerr << "loss2 "<<loss2<<" != 2"<<endl; ret = 2;
        }
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestStraightNonterminal()" << endl; if(TestStraightNonterminal()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestInvertedNonterminal()" << endl; if(TestInvertedNonterminal()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestStraightTerminal()" << endl; if(TestStraightTerminal()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestInvertedTerminal()" << endl; if(TestInvertedTerminal()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestRoot()" << endl; if(TestRoot()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestLossFuzzy Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    Ranks ranks;
    LossFuzzy lf;

};

}

#endif
