#ifndef TEST_FEATURE_SEQUENCE_H__
#define TEST_FEATURE_SEQUENCE_H__

#include "test-base.h"
#include <lader/combined-alignment.h>
#include <lader/feature-sequence.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-set.h>
#include <fstream>

namespace lader {

class TestFeatureSequence : public TestBase {

public:

    TestFeatureSequence() : 
            edge00(0, -1, 0, HyperEdge::EDGE_FOR), 
            edge11(1, -1, 1, HyperEdge::EDGE_FOR), 
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge12t(1, -1, 2, HyperEdge::EDGE_BAC),
            edge12nt(1, 2, 2, HyperEdge::EDGE_INV),
            edge02(0, 1, 2, HyperEdge::EDGE_STR) {
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
    ~TestFeatureSequence() { }

    int TestFeatureDataSequenceParse() {
        // Create the expected value
        vector<string> exp;
        exp.push_back("this");
        exp.push_back("is");
        exp.push_back("a");
        exp.push_back("test");
        // Create the real value
        string str = "this is a test";
        FeatureDataSequence fds;
        fds.FromString(str);
        vector<string> act = fds.GetSequence();
        return CheckVector(exp, act);
    }
    
    int TestFeatureTemplateIsLegal() {
        const int num = 16;
        const char* templ[num] = { "SS", "LN", "RS", "CD", "ET",
                                   "XY", "SD", "CR", "SQE", "LQ1",
                                   "LQE1", "SQ#04", "SS5", "CL",
                                   "SB", "LA"};
        const bool exp[num] = {true, true, true, true, true,
                               false, false, false, false, false,
                               true, true, true, true,
                               true, true};
        int ret = 1;
        for(int i = 0; i < num; i++) {
            if(FeatureSequence::FeatureTemplateIsLegal(templ[i]) != exp[i]) {
                cout << "FeatureTemplateIsLegal failed on " << templ[i] << endl;
                ret = 0;
            }
        }

        {
        const char* templ[num] = { "s0L", "s0R", "l0R", "r0L", "q0",
                                   "q0L", "q0R", "s0", "s01", "s0s",
                                   "s1L", "s1R", "l1R", "r1L",
                                   "s9L", "q9"};
        const bool exp[num] = {true, true, true, true, true,
                               false, false, false, false, false,
                               true, true, true, true,
                               true, true};
        for(int i = 0; i < num; i++) {
            if(FeatureSequence::FeatureTemplateIsLegal(templ[i]) != exp[i]) {
                cout << "FeatureTemplateIsLegal failed on " << templ[i] << endl;
                ret = 0;
            }
        }
        }
        return ret;
    }

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
        edge02r.push_back(MakePair(syms.GetId("A||<s>"     , true), 1));
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

    int TestSequenceFeatures() {
        // Create a dictionary and write it to /tmp/dict.txt
        std::vector<double> hvec(2,0), arvec(2,0);
        hvec[0] = 0.1; hvec[1] = 0.2;
        arvec[0] = 0.3; arvec[1] = 0.4;
        Dictionary dict;
        dict.AddEntry("he", hvec); dict.AddEntry("ate rice", arvec);
        ofstream out("/tmp/dict.txt");
        dict.ToStream(out);
        out.close();
        // Parse the feature sequence
        FeatureSequence feat;
        feat.ParseConfiguration("dict=/tmp/dict.txt,QE%SN%SQE0,Q1%SN%SQ#01");
        // Test for the span
        SymbolSet<int> syms;
        FeatureVectorInt feat00, feat02;
        feat00.push_back(MakePair(syms.GetId("QE||1||+", true), 1));
        feat00.push_back(MakePair(syms.GetId("Q1||1", true), 0.2));
        feat02.push_back(MakePair(syms.GetId("QE||3||-", true), 1));
        // Create the actual features
        FeatureVectorInt edge00act, edge02act;
        feat.GenerateEdgeFeatures(sent, edge00, syms, true, edge00act);
        feat.GenerateEdgeFeatures(sent, edge02, syms, true, edge02act);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(feat00, edge00act);
        ret *= CheckVector(feat02, edge02act);
        return ret;
    }

    int TestEdgeFeatures() {
        FeatureSequence feat;
        feat.ParseConfiguration("D%CD,B%CB,L%CL,D#%CD#,B#%CB#,T%ET");
        // These features apply to non-terminals
        SymbolSet<int> syms;
        FeatureVectorInt edge00exp, edge12exp, edge02exp;
        edge00exp.push_back(MakePair(syms.GetId("T||F", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("D||0", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("B||0", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("L||E", true), 1));
        edge12exp.push_back(MakePair(syms.GetId("T||I", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D||1", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("B||1", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("L||R", true), 1));
        edge02exp.push_back(MakePair(syms.GetId("D#"  , true), 1));
        edge02exp.push_back(MakePair(syms.GetId("B#"  , true), 1));
        edge02exp.push_back(MakePair(syms.GetId("T||S", true), 1));
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

    int TestStateFeatures() {
    	FeatureSequence feat;
    	feat.ParseConfiguration("Q0%q0,LL0%s0L,RR0%s0R,LR0%l0R,RL0%r0L,TL0%t0L,TR0%t0R,T%aT");
    	SymbolSet<int> syms;
    	int n = sent.GetNumWords();
    	vector<FeatureVectorInt> exp(2*n-1, FeatureVectorInt());
    	exp[0].push_back(MakePair(syms.GetId("Q0||he",true),1));
    	exp[0].push_back(MakePair(syms.GetId("LL0||<s>",true),1));
    	exp[0].push_back(MakePair(syms.GetId("RR0||<s>",true),1));
    	exp[0].push_back(MakePair(syms.GetId("TL0||<s>",true),1));
    	exp[0].push_back(MakePair(syms.GetId("TR0||<s>",true),1));
    	exp[0].push_back(MakePair(syms.GetId("T||F",true),1));
    	exp[1].push_back(MakePair(syms.GetId("Q0||ate",true),1));
    	exp[1].push_back(MakePair(syms.GetId("LL0||he",true),1));
    	exp[1].push_back(MakePair(syms.GetId("RR0||he",true),1));
    	exp[1].push_back(MakePair(syms.GetId("TL0||he",true),1));
    	exp[1].push_back(MakePair(syms.GetId("TR0||he",true),1));
    	exp[1].push_back(MakePair(syms.GetId("T||F",true),1));
    	exp[2].push_back(MakePair(syms.GetId("Q0||rice",true),1));
		exp[2].push_back(MakePair(syms.GetId("LL0||ate",true),1));
		exp[2].push_back(MakePair(syms.GetId("RR0||ate",true),1));
		exp[2].push_back(MakePair(syms.GetId("TL0||ate",true),1));
		exp[2].push_back(MakePair(syms.GetId("TR0||ate",true),1));
		exp[2].push_back(MakePair(syms.GetId("T||F",true),1));
    	exp[3].push_back(MakePair(syms.GetId("Q0||</s>",true),1));
		exp[3].push_back(MakePair(syms.GetId("LL0||rice",true),1));
		exp[3].push_back(MakePair(syms.GetId("RR0||rice",true),1));
		exp[3].push_back(MakePair(syms.GetId("TL0||rice",true),1));
		exp[3].push_back(MakePair(syms.GetId("TR0||rice",true),1));
		exp[3].push_back(MakePair(syms.GetId("T||I",true),1));
    	exp[4].push_back(MakePair(syms.GetId("Q0||</s>",true),1));
		exp[4].push_back(MakePair(syms.GetId("LL0||ate",true),1));
		exp[4].push_back(MakePair(syms.GetId("RR0||rice",true),1));
		exp[4].push_back(MakePair(syms.GetId("LR0||ate",true),1));
		exp[4].push_back(MakePair(syms.GetId("RL0||rice",true),1));
		exp[4].push_back(MakePair(syms.GetId("TL0||rice",true),1));
		exp[4].push_back(MakePair(syms.GetId("TR0||ate",true),1));
		exp[4].push_back(MakePair(syms.GetId("T||S",true),1));
		vector<FeatureVectorInt> act(2*n, FeatureVectorInt());
    	DPStateVector stateseq;
    	ActionVector refseq = cal.GetReference();
    	ReordererModel mod;
    	stateseq.push_back(new DPState());
    	for (int step = 1 ; step < 2*n ; step++){
    		DPState * state = stateseq.back();
    		feat.GenerateStateFeatures(sent, *state, refseq[step-1], syms, true, act[step-1]);
//    		cerr << "step " << step-1 << ":";
//    		BOOST_FOREACH(FeaturePairInt f, act[step-1]){
//    			cerr << " " << syms.GetSymbol(f.first);
//    		}
//    		cerr << endl;
    		state->Take(refseq[step-1], stateseq, true);
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	int ret = 1;
    	for (int step = 0 ; step < exp.size() ; step++){
//    		cout << "step " << step << endl;
    		ret *= CheckVector(exp[step], act[step]);
    	}
    	return ret;
    }

    int TestReorderData() {
        FeatureDataSequence data;
        data.FromString("a b c d");
        vector<int> order(4,0);
        order[0] = 2; order[1] = 0; order[2] = 3; order[3] = 1;
        data.Reorder(order);
        string act = data.ToString();
        string exp = "c a d b";
        int ret = 1;
        if(exp != act) {
            cerr << "Reordering failed: act (" << act << ") != " 
                 << " exp (" << exp << ")" << endl;
            ret = 0;
        }
        return ret; 
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestFeatureDataSequenceParse()" << endl; if(TestFeatureDataSequenceParse()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestFeatureTemplateIsLegal()" << endl; if(TestFeatureTemplateIsLegal()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestLeftRightFeatures()" << endl; if(TestLeftRightFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestEdgeFeatures()" << endl; if(TestEdgeFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestStateFeatures()" << endl; if(TestStateFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestReorderData()" << endl; if(TestReorderData()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestSequenceFeatures()" << endl; if(TestSequenceFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestFeatureSequence Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge12t, edge12nt, edge02;
    Ranks cal;
    FeatureDataSequence sent, sent_pos;

};

}

#endif
