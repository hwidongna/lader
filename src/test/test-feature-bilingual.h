/*
 * test-feature-bilingual.h
 *
 *  Created on: Oct 14, 2013
 *      Author: leona
 */




#ifndef TEST_FEATURE_BILINGUAL_H__
#define TEST_FEATURE_BILINGUAL_H__

#include "test-base.h"
#include <lader/combined-alignment.h>
#include <lader/feature-bilingual.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-set.h>
#include <fstream>

namespace lader {

class TestFeatureBilingual : public TestBase {

public:

    TestFeatureBilingual() :
            edge00(0, -1, 0, HyperEdge::EDGE_FOR),
            edge11(1, -1, 1, HyperEdge::EDGE_FOR),
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge12t(1, -1, 2, HyperEdge::EDGE_BAC),
            edge12nt(1, 2, 2, HyperEdge::EDGE_INV),
            edge02(0, 1, 2, HyperEdge::EDGE_STR){
        // Create a combined alignment
        //  x......
        //  ....xxx
        //  ..x....
        vector<string> words(3, "");
        Alignment al(MakePair(3,7));
        al.AddAlignment(MakePair(0,0));
        al.AddAlignment(MakePair(1,4));
        al.AddAlignment(MakePair(1,5));
        al.AddAlignment(MakePair(1,6));
        al.AddAlignment(MakePair(2,2));
        cal = CombinedAlign(words, al);
//        cerr << cal.GetMinF2E(1, 1) << endl;
        ranks = Ranks(cal);
        // Create an English sentence
        f.FromString("he ate rice");
        f_pos.FromString("PRP VBD NN");
        // Create an Korean sentence
		e.FromString("geu neun bab eul meog eoss da");
		e_pos.FromString("NP JX NNG JKO VV EP EF"); // Sejong tag set
    }
    ~TestFeatureBilingual() { }

    int TestLeftRightFeatures() {
        ReordererModel mod;
        FeatureBilingual featl, featr, feata;
        featl.ParseConfiguration("L%LL,R%LR,S%LS,B%LB,A%LA,N%LN");
        featr.ParseConfiguration("L%RL,R%RR,S%RS,B%RB,A%RA,N%RN");
        feata.ParseConfiguration("A%SS%LS%RS,S%SS1");
        // These features apply to only non-terminals
        SymbolSet<int> & syms = mod.GetFeatureIds();
        FeatureVectorInt edge00l, edge02l;
        edge02l.push_back(MakePair(syms.GetId("L||geu" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("R||geu" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("S||geu" , true), 1));
        edge02l.push_back(MakePair(syms.GetId("B||<s>", true), 1));
        edge02l.push_back(MakePair(syms.GetId("A||neun", true), 1));
        edge02l.push_back(MakePair(syms.GetId("N||1"  , true), 1));
        FeatureVectorInt edge00r, edge02r;
        edge02r.push_back(MakePair(syms.GetId("L||bab"     , true), 1));
        edge02r.push_back(MakePair(syms.GetId("R||da"    , true), 1));
        edge02r.push_back(MakePair(syms.GetId("S||bab eul meog eoss da", true), 1));
        edge02r.push_back(MakePair(syms.GetId("B||neun"      , true), 1));
        edge02r.push_back(MakePair(syms.GetId("A||</s>"     , true), 1));
        edge02r.push_back(MakePair(syms.GetId("N||5"       , true), 1));
        FeatureVectorInt edge00a, edge02a;
        edge00a.push_back(MakePair(syms.GetId("S||geu"      , true), 1));
        edge02a.push_back(
            MakePair(syms.GetId("A||geu neun bab eul meog eoss da||geu||bab eul meog eoss da", true), 1));
        edge02a.push_back(MakePair(syms.GetId("S||", true), 1));
        // Create vectors
        FeatureVectorInt edge00lact, edge02lact, edge00ract,
                            edge02ract, edge00aact, edge02aact;
        featl.GenerateBilingualFeatures(f, e, cal, edge00, syms, true, edge00lact);
        featl.GenerateBilingualFeatures(f, e, cal, edge02, syms, true, edge02lact);
        featr.GenerateBilingualFeatures(f, e, cal, edge00, syms, true, edge00ract);
        featr.GenerateBilingualFeatures(f, e, cal, edge02, syms, true, edge02ract);
        feata.GenerateBilingualFeatures(f, e, cal, edge00, syms, true, edge00aact);
        feata.GenerateBilingualFeatures(f, e, cal, edge02, syms, true, edge02aact);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(edge00l, edge00lact);
        ret *= CheckVector(edge02l, edge02lact);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge02l != edge02lact" << endl;
        	fvs = mod.StringifyFeatureVector(edge02l);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge02lact);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        ret *= CheckVector(edge00r, edge00ract);
        ret *= CheckVector(edge02r, edge02ract);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge02r != edge02ract" << endl;
        	fvs = mod.StringifyFeatureVector(edge02r);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge02ract);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        ret *= CheckVector(edge00a, edge00aact);
        ret *= CheckVector(edge02a, edge02aact);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge02a != edge02aact" << endl;
        	fvs = mod.StringifyFeatureVector(edge02a);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge02aact);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        return ret;
    }

    int TestEdgeFeatures() {
    	ReordererModel mod;
        FeatureBilingual feat;
        feat.ParseConfiguration("D%CD,B%CB,L%CL,D#%CD#,B#%CB#,T%ET");
        // These features apply to non-terminals
        SymbolSet<int> & syms = mod.GetFeatureIds();
        FeatureVectorInt edge00exp, edge12exp, edge02exp;
        edge00exp.push_back(MakePair(syms.GetId("T||FC", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("D||2", true), 1)); // abs(1-3)
        edge12exp.push_back(MakePair(syms.GetId("B||-2", true), 1)); // 1-3
        edge12exp.push_back(MakePair(syms.GetId("L||L", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("D#"  , true), 2));
        edge12exp.push_back(MakePair(syms.GetId("B#"  , true), -2));
        edge12exp.push_back(MakePair(syms.GetId("T||IC", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D||4", true), 1)); // abs(5-1)
        edge02exp.push_back(MakePair(syms.GetId("B||4", true), 1)); // 5-1
        edge02exp.push_back(MakePair(syms.GetId("L||R", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D#"  , true), 4));
        edge02exp.push_back(MakePair(syms.GetId("B#"  , true), 4));
        edge02exp.push_back(MakePair(syms.GetId("T||SC", true), 1));
        // Create the actual features
        FeatureVectorInt edge00act, edge12ntact, edge02act;
        feat.GenerateBilingualFeatures(f, e, cal, edge00,   syms, true, edge00act);
        feat.GenerateBilingualFeatures(f, e, cal, edge12nt, syms, true, edge12ntact);
        feat.GenerateBilingualFeatures(f, e, cal, edge02,   syms, true, edge02act);
        // Test the features
        int ret = 1;
        ret *= CheckVector(edge00exp, edge00act);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge00exp != edge00act" << endl;
        	fvs = mod.StringifyFeatureVector(edge00exp);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge00act);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        vector<int> act;
        act.push_back(feat.GetMinF2E('L', edge12nt, cal));
        act.push_back(feat.GetMaxF2E('L', edge12nt, cal));
        act.push_back(feat.GetMinF2E('R', edge12nt, cal));
        act.push_back(feat.GetMaxF2E('R', edge12nt, cal));
        vector<int> exp(4);
        exp[0] = 4; exp[1] = 6; exp[2] = 2; exp[3] = 2;
        ret *= CheckVector(exp, act);
        if (!ret){
        	cerr << "exp != act" << endl;
        }
        ret *= CheckVector(edge12exp, edge12ntact);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge12exp != edge12ntact" << endl;
        	fvs = mod.StringifyFeatureVector(edge12exp);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge12ntact);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        ret *= CheckVector(edge02exp, edge02act);
        if (!ret){
        	FeatureVectorString * fvs;
        	cerr << "edge02exp != edge02act" << endl;
        	fvs = mod.StringifyFeatureVector(edge02exp);
        	cerr << *fvs << endl;
        	delete fvs;
        	fvs = mod.StringifyFeatureVector(edge02act);
        	cerr << *fvs << endl;
        	delete fvs;
        }
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestLeftRightFeatures()" << endl; if(TestLeftRightFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestEdgeFeatures()" << endl; if(TestEdgeFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestFeatureBilingual Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge12t, edge12nt, edge02;
    CombinedAlign cal;
    Ranks ranks;
    FeatureDataSequence f, f_pos;
    FeatureDataSequence e, e_pos;

};

}

#endif
