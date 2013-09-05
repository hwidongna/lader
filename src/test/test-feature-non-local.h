#ifndef TEST_FEATURE_NON_LOCAL_H__
#define TEST_FEATURE_NON_LOCAL_H__

#include "test-base.h"
#include <lader/combined-alignment.h>
#include <lader/feature-non-local.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-set.h>
#include <lader/discontinuous-hyper-edge.h>
#include <fstream>

namespace lader {

class TestFeatureNonLocal : public TestBase {

public:

    TestFeatureNonLocal() :
            edge00(0, -1, 0, HyperEdge::EDGE_FOR), 
            edge11(1, -1, 1, HyperEdge::EDGE_FOR), 
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge12t(1, -1, 2, HyperEdge::EDGE_BAC),
            edge12nt(1, 2, 2, HyperEdge::EDGE_INV),
            edge02(0, 1, 2, HyperEdge::EDGE_STR),
            edge0_2(0, 0, -1, 2, 2, HyperEdge::EDGE_STR),
            edge0_23(0, 0, -1, 2, 3, HyperEdge::EDGE_STR),
            edge01_3(0, 1, -1, 3, 3, HyperEdge::EDGE_STR),
            edge12_4(1, 2, -1, 4, 4, HyperEdge::EDGE_STR),
            edge0_2__3(0, 0, 3, 2, 3, HyperEdge::EDGE_STR),
            edge0__1_3(0, 1, 1, 3, 3, HyperEdge::EDGE_STR),
            edge03(0, 1, 2, 3, 3, HyperEdge::EDGE_STR){
        // Create a combined alignment
        //  x..
        //  ..x
        //  .x.
        vector<string> words(3, "x");
        Alignment al(MakePair(3,3));
        al.AddAlignment(MakePair(0,0));
        al.AddAlignment(MakePair(1,2));
        al.AddAlignment(MakePair(2,1));
        cal = Ranks(CombinedAlign(words,al));
        // Create a sentence
        string str = "he ate rice";
        sent.FromString(str);
        string str_pos = "PRP VBD NN";
        sent_pos.FromString(str_pos);
    }
    ~TestFeatureNonLocal() { }

    int TestLeftRightFeatures() {
        ReordererModel mod;
        FeatureSequence featl, featr, feata;
        featl.ParseConfiguration("L%LL,R%LR,S%LS,B%LB,A%LA,N%LN");
        featr.ParseConfiguration("L%RL,R%RR,S%RS,B%RB,A%RA,N%RN");
        feata.ParseConfiguration("A%SS%LS%RS,S%SS1");
        // These features apply to only non-terminals
        SymbolSet<int> syms;
        FeatureVectorInt edge00l, edge02l;
        edge02l.push_back(MakePair(syms.GetId("L||he" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("R||he" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("S||he" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("B||<s>", true), 1));
        edge02l.push_back(MakePair(syms.GetId("A||ate", true), 1));
        edge02l.push_back(MakePair(syms.GetId("N||1"  , true), 1));
        FeatureVectorInt edge00r, edge02r;
        edge02r.push_back(MakePair(syms.GetId("L||ate"     , true), 1));
        edge02r.push_back(MakePair(syms.GetId("R||rice"    , true), 1));
        edge02r.push_back(MakePair(syms.GetId("S||ate rice", true), 1));
        edge02r.push_back(MakePair(syms.GetId("B||he"      , true), 1));
        edge02r.push_back(MakePair(syms.GetId("A||</s>"     , true), 1));
        edge02r.push_back(MakePair(syms.GetId("N||2"       , true), 1));
        FeatureVectorInt edge00a, edge02a;
        edge00a.push_back(MakePair(syms.GetId("S||he"      , true), 1));
        edge02a.push_back(
            MakePair(syms.GetId("A||he ate rice||he||ate rice", true), 1));
        edge02a.push_back(MakePair(syms.GetId("S||", true), 1));
        // Create vectors
        FeatureVectorInt edge00lact, edge02lact, edge00ract,
                            edge02ract, edge00aact, edge02aact;
        featl.GenerateEdgeFeatures(sent, edge00, syms, true, edge00lact);
        featl.GenerateEdgeFeatures(sent, edge02, syms, true, edge02lact);
        featr.GenerateEdgeFeatures(sent, edge00, syms, true, edge00ract);
        featr.GenerateEdgeFeatures(sent, edge02, syms, true, edge02ract);
        feata.GenerateEdgeFeatures(sent, edge00, syms, true, edge00aact);
        feata.GenerateEdgeFeatures(sent, edge02, syms, true, edge02aact);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(edge00l, edge00lact);
        ret *= CheckVector(edge02l, edge02lact);
        ret *= CheckVector(edge00r, edge00ract);
        ret *= CheckVector(edge02r, edge02ract);
        ret *= CheckVector(edge00a, edge00aact);
        ret *= CheckVector(edge02a, edge02aact);
        return ret;
    }

    int TestEdgeFeatures() {
        FeatureSequence feat;
        feat.ParseConfiguration("D%CD,B%CB,L%CL,D#%CD#,B#%CB#,T%ET");
        // These features apply to non-terminals
        SymbolSet<int> syms;
        FeatureVectorInt edge00exp, edge12exp, edge02exp;
        edge00exp.push_back(MakePair(syms.GetId("T||FC", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("D||0", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("B||0", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("L||E", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("T||IC", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D||1", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("B||1", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("L||R", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D#"  , true), 1));
        edge02exp.push_back(MakePair(syms.GetId("B#"  , true), 1));
        edge02exp.push_back(MakePair(syms.GetId("T||SC", true), 1));
        // Create the actual features
        FeatureVectorInt edge00act, edge12ntact, edge02act;
        feat.GenerateEdgeFeatures(sent, edge00,   syms, true, edge00act);
        feat.GenerateEdgeFeatures(sent, edge12nt, syms, true, edge12ntact);
        feat.GenerateEdgeFeatures(sent, edge02,   syms, true, edge02act);
        // Test the features
        int ret = 1;
        ret *= CheckVector(edge00exp, edge00act);
        ret *= CheckVector(edge12exp, edge12ntact);
        ret *= CheckVector(edge02exp, edge02act);
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestLeftRightFeatures()" << endl; if(TestLeftRightFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestEdgeFeatures()" << endl; if(TestEdgeFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestFeatureNonLocal Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge12t, edge12nt, edge02;
    DiscontinuousHyperEdge edge0_2, edge0_23, edge01_3, edge12_4, edge0_2__3, edge0__1_3, edge03;
    Ranks cal;
    FeatureDataSequence sent, sent_pos;

};

}

#endif
