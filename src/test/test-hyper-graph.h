#ifndef TEST_HYPER_GRAPH_H__
#define TEST_HYPER_GRAPH_H__

#include "test-base.h"
#include <climits>
#include <lader/hyper-graph.h>
#include <lader/alignment.h>
#include <lader/reorderer-model.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-sequence.h>
#include <lader/feature-non-local.h>
#include <lader/feature-set.h>
#include <lader/ranks.h>
#include <lader/hyper-graph.h>
#include <fstream>
#include <time.h>

namespace lader {

class TestHyperGraph : public TestBase {

public:

    TestHyperGraph() : 
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
        vector<string> words(3,"x");
        Alignment al(MakePair(3,3));
        al.AddAlignment(MakePair(0,0));
        al.AddAlignment(MakePair(1,2));
        al.AddAlignment(MakePair(2,1));
        cal = CombinedAlign(words, al);
        // Create a sentence
        string str = "he ate rice";
        sent.FromString(str);
        string str_pos = "PRP VBD NN";
        sent_pos.FromString(str_pos);
        // Create a reorderer model with 4 weights
        model.SetWeight("W1", 1);
        model.SetWeight("W2", 2);
        model.SetWeight("W3", 3);
        model.SetWeight("W4", 4);
        // Set up the feature generators
        featw = new FeatureSequence;
        featp = new FeatureSequence;
        featw->ParseConfiguration("SW%LS%RS");
        featp->ParseConfiguration("SP%LS%RS");
        // Set up the feature set
        set.AddFeatureGenerator(featw);
        set.AddFeatureGenerator(featp);
        // Set up the data
        datas.push_back(&sent);
        datas.push_back(&sent_pos);

        // ------ Make a hypergraph for testing various things ------
        // Make the target side spans
        ts00 = new TargetSpan(0,0);
        ts01 = new TargetSpan(0,1);
        ts11 = new TargetSpan(1,1);
        tsr = new TargetSpan(0,1);
        // Add the hypotheses
        ts00->AddHypothesis(new Hypothesis(1,-1,0, 0,0,0,0,HyperEdge::EDGE_FOR));
        ts11->AddHypothesis(new Hypothesis(2,-1,0, 1,1,1,1,HyperEdge::EDGE_FOR));
        ts01->AddHypothesis(new Hypothesis(5,-1,0, 0,1,1,0,HyperEdge::EDGE_INV,1,0,0,ts00,ts11));
        ts01->AddHypothesis(new Hypothesis(4,-1,0, 0,1,0,1,HyperEdge::EDGE_FOR));
        ts01->AddHypothesis(new Hypothesis(3,-1,0, 0,1,0,1,HyperEdge::EDGE_STR,1,0,0,ts00,ts11));
        tsr->AddHypothesis(new Hypothesis(6,0,0, 1,0,-1,2,HyperEdge::EDGE_ROOT,-1,0,-1,ts01));
        tsr->AddHypothesis(new Hypothesis(6,0,0, 0,1,-1,2,HyperEdge::EDGE_ROOT,-1,1,-1,ts01));
        tsr->AddHypothesis(new Hypothesis(6,0,0, 0,1,-1,2,HyperEdge::EDGE_ROOT,-1,2,-1,ts01));
        // Add the features
        FeatureVectorInt 
            *fv00 = new FeatureVectorInt(1, MakePair(1,1)),
            *fv11 = new FeatureVectorInt(1, MakePair(2,1)),
            *fv01f = new FeatureVectorInt(1,MakePair(4,1)),
            *fv01s = new FeatureVectorInt(1,MakePair(3,1)),
            *fv01b = new FeatureVectorInt(1,MakePair(5,1));
        fv00->push_back(MakePair(10,1));
        fv11->push_back(MakePair(10,1));
        fv01f->push_back(MakePair(10,1));
        fv01s->push_back(MakePair(10,1));
        fv01b->push_back(MakePair(10,1));
        ts00->SaveStraightFeautures(-1, fv00);
        ts11->SaveStraightFeautures(-1, fv11);
        ts01->SaveStraightFeautures(-1, fv01f);
        ts01->SaveStraightFeautures(1, fv01s);
        ts01->SaveInvertedFeautures(1, fv01b);
        // features are saved in stack
        my_hg.SaveFeatures(true);
//        my_hg.SetEdgeFeatures(HyperEdge(0,-1,0,HyperEdge::EDGE_FOR), fv00);
//        my_hg.SetEdgeFeatures(HyperEdge(1,-1,1,HyperEdge::EDGE_FOR), fv11);
//        my_hg.SetEdgeFeatures(HyperEdge(0,-1,1,HyperEdge::EDGE_FOR), fv01f);
//        my_hg.SetEdgeFeatures(HyperEdge(0,1,1,HyperEdge::EDGE_STR), fv01s);
//        my_hg.SetEdgeFeatures(HyperEdge(0,1,1,HyperEdge::EDGE_INV), fv01b);
        // Set the stacks
        my_hg.SetStack(0,0,ts00);
        my_hg.SetStack(0,1,ts01);
        my_hg.SetStack(1,1,ts11);
        my_hg.SetStack(0,2,tsr); // Abusing SetStack to set the root
//        BOOST_FOREACH(SpanStack * stack, my_hg.GetStacks())
//			BOOST_FOREACH(HypothesisStack * trg, stack->GetSpans())
//				BOOST_FOREACH(Hypothesis * hyp, trg->GetHypotheses())
//					cerr << "Hypothesis " << *hyp << endl;
        // Add the loss
        ts00->GetHypothesis(0)->SetLoss(1);
        ts11->GetHypothesis(0)->SetLoss(2);
        ts01->GetHypothesis(0)->SetLoss(5);
        ts01->GetHypothesis(1)->SetLoss(4);
        ts01->GetHypothesis(2)->SetLoss(3);
        tsr->GetHypothesis(0)->SetLoss(6);
        // // Sort the stacks so we get the best value first
        // BOOST_FOREACH(SpanStack & stack, my_hg.GetStacks())
        //     stack.SortSpans(true);
    }

    int TestGetTrgSpanID() {
        vector<pair<int,int> > in;
        in.push_back(MakePair(0,0));
        in.push_back(MakePair(0,1));
        in.push_back(MakePair(1,1));
        in.push_back(MakePair(0,2));
        in.push_back(MakePair(1,2));
        in.push_back(MakePair(2,2));
        int ret = 1;
        HyperGraph hg;
        for(int i = 0; i < (int)in.size(); i++) {
            if(hg.GetTrgSpanID(in[i].first, in[i].second) != i) {
                cerr << "hg.GetTrgSpanID("<<in[i].first<<", "<<in[i].second<<") == " << hg.GetTrgSpanID(in[i].first, in[i].second) << endl; ret = 0;
            }
        }
        return ret;
    }

    int TestGetEdgeFeaturesAndWeights() {
        // Make a reorderer model
        ReordererModel mod;
        // Test that these features are made properly
        FeatureVectorString edge02exp;
        edge02exp.push_back(MakePair(string("SW||he||ate rice"), 1));
        edge02exp.push_back(MakePair(string("SP||PRP||VBD NN"), 1));
        // Make the hypergraph and get the features
        HyperGraph hyper_graph;
        // -save_features
        hyper_graph.SaveFeatures(true);
        // for Save{Striaght,Inverted}Features
        TargetSpan * span02 = new TargetSpan(0, 2);
        hyper_graph.SetStack(0, 2, span02);
        // Generate the features
        const FeatureVectorInt * edge02int = 
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge02);
        FeatureVectorString * edge02act =
                        mod.StringifyFeatureVector(*edge02int);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(edge02exp, *edge02act);

        FeatureVectorInt edge02intexp;
        edge02intexp.push_back(MakePair(mod.GetFeatureIds().GetId("SW||he||ate rice"), 1));
        edge02intexp.push_back(MakePair(mod.GetFeatureIds().GetId("SP||PRP||VBD NN"), 1));
        ret *= CheckVector(edge02intexp, *edge02int);
        // Generate the features again
        const FeatureVectorInt * edge02int2 = 
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge02);
        // Make sure that the pointers are equal
        if(edge02int != edge02int2) {
            cerr << "Edge pointers are not equal." << endl;
            ret = 0;
        }
        mod.SetWeight("SW||he||ate rice", 1);
        mod.SetWeight("SP||PRP||VBD NN", 2);
        // Check to make sure that the weights are Ok
        double weight_act = hyper_graph.GetEdgeScore(mod, set,
                                                     datas, edge02);
        if(weight_act != 1+2) {
            cerr << "Weight is not the expected 3: "<<weight_act<<endl;
            ret = 0;
        }
        return ret;
    }

    int TestGetNonLocalFeaturesAndWeights() {
        // Make a reorderer model
        ReordererModel mod;
        FeatureNonLocal *non_local_featw, *non_local_featp;
        // Set up the feature generators
        non_local_featw = new FeatureNonLocal;
        non_local_featp = new FeatureNonLocal;
        non_local_featw->ParseConfiguration("TO%SL%SR");
        non_local_featp->ParseConfiguration("TI%LR%RL");
        // Set up the feature set
        FeatureSet non_local_set;
        non_local_set.AddFeatureGenerator(non_local_featw);
        non_local_set.AddFeatureGenerator(non_local_featp);
        // Make the hypergraph and get the features
        HyperGraph graph;
        // Create spans, and generate hypotheses
        TargetSpan *stack00 = new TargetSpan(0,0)
        		, *stack11 = new TargetSpan(1,1);
        stack00->AddHypothesis(new Hypothesis(1,1,0, 0,0,0,0, HyperEdge::EDGE_FOR));
        graph.SetStack(0, 0, stack00);
        graph.SetStack(1, 1, stack11);
        TargetSpan *stack01 = new TargetSpan(0,1);
        stack01->AddHypothesis(new Hypothesis(2,2,0, 0,1,0,1, HyperEdge::EDGE_FOR));
        TargetSpan *stack22 = new TargetSpan(2,2);
		stack22->AddHypothesis(new Hypothesis(3,3,0, 2,2,2,2, HyperEdge::EDGE_FOR));
		graph.SetStack(2, 2, stack22);
		graph.SetStack(0, 1, stack01);
		TargetSpan *stack12 = new TargetSpan(1,2);
		stack12->AddHypothesis(new Hypothesis(5,4,1, 1,2,2,1, HyperEdge::EDGE_BAC));
		graph.SetStack(1, 2, stack12); // just for testing
        set.SetMaxTerm(1);
		TargetSpan *stack02 = new TargetSpan(0,2);
		graph.SetStack(0, 2, stack02);
		graph.ProcessOneSpan(mod, set, non_local_set, datas, 0, 2, 100);
        // The stack should contain four hypotheses: S(0,21), I(21,0), S(01,2), I(2,01)
        int ret = 1;
        if(stack02->size() != 4) {
            cerr << "stack02->size() != 4: " << stack02->size() << endl; ret = 0;
        }
        if(!ret) return 0;
        Hypothesis *hyp021 = stack02->GetHypothesis(0);
        // Generate the features
        const FeatureVectorInt * hyp021int =
        		non_local_set.MakeNonLocalFeatures(datas,
        				*hyp021->GetLeftHyp(), *hyp021->GetRightHyp(),
        				mod.GetFeatureIds(), false);
        FeatureVectorString * hyp021act =
                        mod.StringifyFeatureVector(*hyp021int);
        // Do the parsing and checking
        // Test that these features are made properly
        FeatureVectorString hyp021exp;
        hyp021exp.push_back(MakePair(string("TO||he||ate"), 1));
        hyp021exp.push_back(MakePair(string("TI||PRP||NN"), 1));
        ret *= CheckVector(hyp021exp, *hyp021act);
        FeatureVectorInt hyp021intexp;
        hyp021intexp.push_back(MakePair(mod.GetFeatureIds().GetId("TO||he||ate"), 1));
        hyp021intexp.push_back(MakePair(mod.GetFeatureIds().GetId("TI||PRP||NN"), 1));
        ret *= CheckVector(hyp021intexp, *hyp021int);
        mod.SetWeight("TO||he||ate", 1);
        mod.SetWeight("TI||PRP||NN", 2);
        // Check to make sure that the weights are Ok
        double weight_act = graph.GetNonLocalScore(mod, non_local_set,
                                      datas, hyp021->GetEdge(), hyp021->GetLeftHyp(), hyp021->GetRightHyp());
        if(weight_act != 1+2) {
            cerr << "Weight is not the expected 3: "<<weight_act<<endl;
            ret = 0;
        }

        stack11->AddHypothesis(new Hypothesis(1,1,0, 1,1,1,1, HyperEdge::EDGE_FOR));
        stack01->ClearHypotheses();
        graph.ProcessOneSpan(mod, set, non_local_set, datas, 0, 1, 100);
        // The stack should contain two hypotheses: S(0,1), I(1,0)
        if(stack01->size() != 2) {
        	cerr << "stack01->size() != 2: " << stack01->size() << endl; ret = 0;
        }
//        Hypothesis *hyp01 = stack01processed->GetHypothesis(0);
        Hypothesis *hyp10 = stack01->GetHypothesis(1);
        if (hyp10->GetEdgeType() != HyperEdge::EDGE_INV){
        	cerr << "hyp10->GetEdgeType() != HyperEdge::EDGE_INV" << endl; ret = 0;
        }
        FeatureVectorString hyp10exp;
        hyp10exp.push_back(MakePair(string("TO||ate||he"), 1));
        hyp10exp.push_back(MakePair(string("TI||VBD||PRP"), 1));
        // Generate the features
        const FeatureVectorInt * hyp10int =
        		non_local_set.MakeNonLocalFeatures(datas,
        				*hyp10->GetRightHyp(), *hyp10->GetLeftHyp(),
        				mod.GetFeatureIds(), false);
        FeatureVectorString * hyp10act =
        		mod.StringifyFeatureVector(*hyp10int);
        // Do the parsing and checking
        ret *= CheckVector(hyp10exp, *hyp10act);
        return ret;
    }

    // Test the processing of a single span
    int TestProcessOneSpan() {
        HyperGraph graph;
        // Create two spans for 00 and 11, so we can process 01
        TargetSpan *stack00 = new TargetSpan(0,0)
        		, *stack11 = new TargetSpan(1,1);
        stack00->AddHypothesis(new Hypothesis(1,1,0, 0,0,0,0, HyperEdge::EDGE_FOR));
        graph.SetStack(0, 0, stack00);
        stack11->AddHypothesis(new Hypothesis(2,2,0, 1,1,1,1, HyperEdge::EDGE_FOR));
        graph.SetStack(1, 1, stack11);
        // Try processing 01
        set.SetMaxTerm(0);
    	set.SetUseReverse(true);
    	TargetSpan *stack01 = new TargetSpan(0, 1);
    	graph.SetStack(0, 1, stack01);
        graph.ProcessOneSpan(model, set, non_local_set, datas, 0, 1, 100);
        // The stack should contain four hypotheses: S(0,1), I(1,0), F(01), B(10)
        int ret = 1;
        if(stack01->size() != 4) {
            cerr << "stack01->size() != 4: " << stack01->size() << endl; ret = 0;
        }
        if(!ret) return 0;
        // Check to make sure that the scores are in order
        vector<double> score_exp(4,0), score_act(4);
        score_exp[0] = 3; score_exp[1] = 3;
        score_act[0] = stack01->GetHypothesis(0)->GetScore();
        score_act[1] = stack01->GetHypothesis(1)->GetScore();
        score_act[2] = stack01->GetHypothesis(2)->GetScore();
        score_act[3] = stack01->GetHypothesis(3)->GetScore();
        ret = CheckVector(score_exp, score_act);

		// Check to make sure that pruning works
		stack01->ClearHypotheses();
		graph.ProcessOneSpan(model, set, non_local_set, datas, 0, 1, 3);
		if(stack01->size() != 3) {
			cerr << "stack01->size() != 3: " << stack01->size() << endl; ret = 0;
		}

        TargetSpan *stack22 = new TargetSpan(2,2);
		stack22->AddHypothesis(new Hypothesis(3,3,0, 2,2,2,2, HyperEdge::EDGE_FOR));
		graph.SetStack(2, 2, stack22);
//		graph.SetStack(0, 1, stack01);
		TargetSpan *stack12 = new TargetSpan(1,2);
		stack12->AddHypothesis(new Hypothesis(4,4,0, 1,2,2,1, HyperEdge::EDGE_BAC));
		graph.SetStack(1, 2, stack12); // just for testing
		TargetSpan *stack02 = new TargetSpan(0, 2);
		graph.SetStack(0, 2, stack02);
		graph.ProcessOneSpan(model, set, non_local_set, datas, 0, 2, 100);
        // The number of hypothesis should be 3! + 1
		if(stack02->size() != 3*2 + 1) {
			cerr << "stack02->size() != 7: " << stack02->size() << endl; ret = 0;
			BOOST_FOREACH(const Hypothesis *hyp, stack02->GetHypotheses()){
				cerr << *hyp;
				if (!hyp->IsTerminal())
					hyp->PrintChildren(cerr);
				else
					cerr << endl;
			}
		}

        // delete stack00; delete stack01;
        // delete stack11; delete stack01pruned;
        return ret;
    }

    int TestBuildHyperGraph() {
        HyperGraph graph;
        set.SetMaxTerm(0);
        set.SetUseReverse(false);
        ReordererModel model;
        graph.BuildHyperGraph(model, set, non_local_set, datas);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        // The total number of stacks should be 7: 0-0 0-1 1-1 0-2 1-2 2-2 root
        if(stacks.size() != 7) {
            cerr << "stacks.size() != 7: " << stacks.size() << endl; ret = 0;
            // The number of hypothesis should be 2! + 1
        } else if (graph.GetStack(0,1)->size() != 2 + 1) {
        	cerr << "stacks[0,1]->size() != 3: " <<graph.GetStack(0,1)->size()<< endl;
        	BOOST_FOREACH(const Hypothesis *hyp, graph.GetStack(0,1)->GetHypotheses()){
        		cerr << *hyp <<  endl;
        	}
        	ret = 0;
            // The number of hypothesis should be 2! + 1
        } else if (graph.GetStack(1,2)->size() != 2 + 1) {
        	cerr << "stacks[1,2]->size() != 3: " <<graph.GetStack(1,2)->size()<< endl;
        	BOOST_FOREACH(const Hypothesis *hyp, graph.GetStack(1,2)->GetHypotheses()){
        		cerr << *hyp <<  endl;
        	}
        	ret = 0;
        	// The number of hypothesis should be 3! + 1
        } else if (stacks[3]->size() != 3*2 + 1) {
        	cerr << "Root node stacks[3]->size() != 7: " <<stacks[3]->size()<< endl;
        	BOOST_FOREACH(const Hypothesis *hyp, stacks[3]->GetHypotheses()){
        		cerr << *hyp <<  endl;
        	}
        	ret = 0;
        } else if (stacks[6]->size() != stacks[3]->size()) {
            cerr << "Root hypotheses " << stacks[6]->size()
                 << " and root spans " << stacks[3]->size() << " don't match." <<
                 endl; ret = 0;
        }
        set.SetUseReverse(true);
        return ret;
    }

    int TestBuildHyperGraphCubeGrowing() {
        HyperGraph graph(true);
        set.SetMaxTerm(0);
        set.SetUseReverse(false);
        ReordererModel model;
        graph.BuildHyperGraph(model, set, non_local_set, datas);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        // The total number of stacks should be 7: 0-0 0-1 1-1 0-2 1-2 2-2 root
        if(stacks.size() != 7) {
            cerr << "stacks.size() != 7: " << stacks.size() << endl; ret = 0;
		// The number of hypothesis should be 3! + 1
        } else if (stacks[3]->size() != 3*2 + 1) {
            cerr << "Root node stacks[3]->size() != 7: " <<stacks[3]->size()<< endl;
            BOOST_FOREACH(const Hypothesis *hyp, stacks[3]->GetHypotheses()){
                cerr << *hyp <<  endl;
            }
            ret = 0;
        } else if (stacks[6]->size() != stacks[3]->size()) {
            cerr << "Root hypotheses " << stacks[6]->size()
                 << " and root spans " << stacks[3]->size() << " don't match." <<
                 endl; ret = 0;
        }
        set.SetUseReverse(true);
        return ret;
    }

    int TestBuildHyperGraphMultiThreads() {
    	HyperGraph graph(true);
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
        featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
        FeatureDataSequence sent;
        sent.FromString("t h i s i s a v e r y v e r y v e r y v e r y l o n g s e n t e n c e .");
        vector<FeatureDataBase*> datas;
        datas.push_back(&sent);
    	struct timespec tstart={0,0}, tend={0,0};
        int beam_size = 100;
        graph.SetBeamSize(beam_size);
        ReordererModel model;

    	graph.SetThreads(1);
        clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(model, set, non_local_set, datas);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		double thread1 = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
				((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);

		// reusing model requires no more model->GetFeatureIds()->GetId(add=true)
		// therefore, it will be much faster
    	graph.SetThreads(4);
    	graph.ClearStacks();
        clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(model, set, non_local_set, datas);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		double thread4 = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
				((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    	const std::vector<TargetSpan*> & stacks = graph.GetStacks();
    	int ret = 1;
    	int n = sent.GetNumWords();
    	// The total number of stacks should be n*(n+1)/2 + 1
    	if(stacks.size() != n*(n+1)/2 + 1) {
    		cerr << "stacks.size() != " << n*(n+1)/2 + 1 << ": " << stacks.size() << endl; ret = 0;
    		// The number of hypothesis should be beam_size
    	} else if (graph.GetRoot()->size() != beam_size) {
    		cerr << "root stacks->size() != " << beam_size << ": " <<graph.GetRoot()->size()<< endl;
    		BOOST_FOREACH(const Hypothesis *hyp, graph.GetRoot()->GetHypotheses()){
    			cerr << *hyp <<  endl;
    		}
    		ret = 0;
    	}
		if(thread4 > thread1){
			cerr << "more threads, more time? " << thread4 << " > " << thread1 << endl;
			ret = 0;
		}
    	set.SetUseReverse(true);
    	return ret;
    }

    int TestBuildHyperGraphSaveFeatures() {
    	HyperGraph graph(true);
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
    	featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
    	FeatureDataSequence sent;
    	sent.FromString("t h i s i s a v e r y v e r y v e r y v e r y l o n g s e n t e n c e .");
    	vector<FeatureDataBase*> datas;
    	datas.push_back(&sent);
    	struct timespec tstart={0,0}, tend={0,0};
    	int beam_size = 100;
        graph.SetBeamSize(beam_size);
    	ReordererModel model;

    	graph.SaveFeatures(true);
    	clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(model, set, non_local_set, datas);
    	clock_gettime(CLOCK_MONOTONIC, &tend);
    	double before_save = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
    			((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);

    	// reusing saved features does not access to model->GetFeatureIds() anymore
    	// therefore, it will be much faster
    	ReordererModel empty_model; // use empty model
    	empty_model.SetAdd(false); // do not allow adding feature ids anymore
    	clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(empty_model, set, non_local_set, datas);
    	clock_gettime(CLOCK_MONOTONIC, &tend);
    	double after_save = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
    			((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
    	const std::vector<TargetSpan*> & stacks = graph.GetStacks();
    	int ret = 1;
    	int n = sent.GetNumWords();
    	// The total number of stacks should be n*(n+1)/2 + 1
    	if(stacks.size() != n*(n+1)/2 + 1) {
    		cerr << "stacks.size() != " << n*(n+1)/2 + 1 << ": " << stacks.size() << endl; ret = 0;
    		// The number of hypothesis should be beam_size
    	} else if (graph.GetRoot()->size() != beam_size) {
    		cerr << "root stacks->size() != " << beam_size << ": " <<graph.GetRoot()->size()<< endl;
    		BOOST_FOREACH(const Hypothesis *hyp, graph.GetRoot()->GetHypotheses()){
    			cerr << *hyp <<  endl;
    		}
    		ret = 0;
    	}
    	if(after_save > before_save){
    		cerr << "save features, more time? " << after_save << " > " << before_save << endl;
    		ret = 0;
    	}
    	set.SetUseReverse(true);
    	return ret;
    }
    int TestAccumulateLoss() {
        // The value of the loss should be 1+2+5+6 = 14 (3 and 4 are not best)
        double val = tsr->GetHypothesis(0)->AccumulateLoss();
        int ret = 1;
        if(val != 14) {
            cerr << "tsr->AccumulateLoss() != 14: " <<
                     tsr->GetHypothesis(0)->AccumulateLoss() << endl; ret = 0;
        }
        return ret;
    }

    int TestAccumulateFeatures() {
        // The value of the loss should be 1:1, 2:1, 5:1, 10:4
        FeatureVectorInt act;
        std::tr1::unordered_map<int, double> feat_map;
        my_hg.AccumulateFeatures(feat_map, model, set, non_local_set, datas, my_hg.GetBest());
        ClearAndSet(act, feat_map);
        FeatureVectorInt exp;
        exp.push_back(MakePair(1,1));
        exp.push_back(MakePair(2,1));
        exp.push_back(MakePair(5,1));
        exp.push_back(MakePair(10,3));
        // Test the rescoring
        return CheckVector(exp, act);
    }

    // Test that rescoring works
    int TestRescore() {
//        // Create a model that assigns a weight of -1 to each production
//        ReordererModel mod;
//        for(int i = 0; i < 20; i++) {
//            ostringstream oss;
//            oss << "WEIGHT" << i;
//            mod.SetWeight(oss.str(), 0);
//        }
//        mod.SetWeight("WEIGHT10", -1);
        int ret = 1;
        // Simply rescoring with this model should pick the forward production
        // with a score of -1
        double score = my_hg.Rescore(0.0);
        if(score != -1) {
            cerr << "Rescore(mod, 0.0) != -1: " << score << endl; ret = 0;
        }
        // Rescoring with loss +1 should pick the inverted terminal
        // with a loss of 14, minus a weight of 3 -> 11
        score = my_hg.Rescore(1.0);
        if(score != 11) {
            cerr << "Rescore(mod, 1.0) != 11: " << score << endl; ret = 0;
        }
        return ret;
    }

    // Test various types of reordering in the hypergraph
    int TestReorderingAndPrint() {
        // Create the expected reordering vectors
        vector<int> vec01(2,0); vec01[0] = 0; vec01[1] = 1;
        vector<int> vec10(2,0); vec10[0] = 1; vec10[1] = 0;
        vector<string> str01(2); str01[0] = "0"; str01[1] = "1";
        // Create a forest that can handle various things
        TargetSpan *span00 = new TargetSpan(0,0),
                   *span01 = new TargetSpan(0,1),
                   *span11 = new TargetSpan(1,1),
                   *spanr = new TargetSpan(0,1);
        span00->AddHypothesis(new Hypothesis(1,1,0,0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(new Hypothesis(1,1,0,1,1,-1,-1,HyperEdge::EDGE_FOR));
        span01->AddHypothesis(new Hypothesis(1,1,0,0,1,-1,-1,HyperEdge::EDGE_FOR));
        spanr->AddHypothesis(new Hypothesis(1,1,0,0,1,-1,-1,HyperEdge::EDGE_ROOT,-1,0,-1,span01));
        HyperGraph graph;
        graph.SetStack(0,0, span00);
        graph.SetStack(1,1, span11);
        graph.SetStack(0,1, span01);
        graph.SetStack(0,2, spanr);
        // Get the reordering for forward
        int ret = 1;
        vector<int> for_reorder;
        graph.GetBest()->GetReordering(for_reorder);
         ostringstream for_oss; spanr->GetHypothesis(0)->PrintParse(str01, for_oss);
         ret = min(ret, CheckVector(vec01, for_reorder));
         ret = min(ret, CheckString("(F (FW 0) (FW 1))", for_oss.str()));
         // Get the reordering bac backward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_BAC);
         vector<int> bac_reorder;
         graph.GetBest()->GetReordering(bac_reorder);
         ostringstream bac_oss; spanr->GetHypothesis(0)->PrintParse(str01, bac_oss);
         ret = min(ret, CheckVector(vec10, bac_reorder));
         ret = min(ret, CheckString("(B (BW 0) (BW 1))", bac_oss.str()));
         // Get the reordering for forward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_STR);
         span01->GetHypothesis(0)->SetLeftChild(span00);
         span01->GetHypothesis(0)->SetLeftRank(0);
         span01->GetHypothesis(0)->SetRightChild(span11);
         span01->GetHypothesis(0)->SetRightRank(0);
         vector<int> str_reorder;
         graph.GetBest()->GetReordering(str_reorder);
         ostringstream str_oss; spanr->GetHypothesis(0)->PrintParse(str01, str_oss);
         ret = min(ret, CheckVector(vec01, str_reorder));
         ret = min(ret,CheckString("(S (F (FW 0)) (F (FW 1)))",str_oss.str()));
         // Get the reordering for forward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_INV);
         vector<int> inv_reorder;
         graph.GetBest()->GetReordering(inv_reorder);
         ostringstream inv_oss;spanr->GetHypothesis(0)->PrintParse(str01, inv_oss);
         ret = min(ret, CheckVector(vec10, inv_reorder));
         ret = min(ret,CheckString("(I (F (FW 0)) (F (FW 1)))",inv_oss.str()));
        return ret;
    }

    // Test various types of reordering in the hypergraph
    int TestPrintHyperGraph() {
        // Create the expected reordering vectors
        vector<string> str01(2); str01[0] = "0"; str01[1] = "1";
        // Create a forest that can handle various things
        TargetSpan *span00 = new TargetSpan(0,0),
                   *span01 = new TargetSpan(0,1),
                   *span11 = new TargetSpan(1,1),
                   *spanr = new TargetSpan(0,1);
        span00->AddHypothesis(new Hypothesis(1,1,0,0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(new Hypothesis(1,1,0,1,1,-1,-1,HyperEdge::EDGE_BAC));
        span01->AddHypothesis(new Hypothesis(1,1,0,0,1,-1,-1,HyperEdge::EDGE_INV,-1,1,-1,span00,span11));
        spanr->AddHypothesis(new Hypothesis(1,1,0,0,1,-1,-1,HyperEdge::EDGE_ROOT,-1,0,-1,span01));
        // Get the reordering for forward
        int ret = 1;
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_FOR);
        ret = min(ret, CheckString("[R] ||| [F]",
            spanr->GetHypothesis(0)->GetRuleString(str01, 'F')));
        ret = min(ret, CheckString("[F] ||| 0 1",
            span01->GetHypothesis(0)->GetRuleString(str01)));
        // Get the reordering bac backward
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_BAC);
        ret = min(ret, CheckString("[R] ||| [B]",
            spanr->GetHypothesis(0)->GetRuleString(str01, 'B')));
        ret = min(ret, CheckString("[B] ||| 0 1",
            span01->GetHypothesis(0)->GetRuleString(str01)));
        // Get the reordering for forward
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_STR);
        span01->GetHypothesis(0)->SetLeftChild(span00);
        span01->GetHypothesis(0)->SetRightChild(span11);
        ret = min(ret, CheckString("[R] ||| [S]",
            spanr->GetHypothesis(0)->GetRuleString(str01, 'S')));
        ret = min(ret, CheckString("[S] ||| [F] [B]",
            span01->GetHypothesis(0)->GetRuleString(str01, 'F', 'B')));
        ret = min(ret, CheckString("[F] ||| 0",
            span00->GetHypothesis(0)->GetRuleString(str01)));
        ret = min(ret, CheckString("[B] ||| 1",
            span11->GetHypothesis(0)->GetRuleString(str01)));
        // Get the reordering for forward
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_INV);
        ret = min(ret, CheckString("[R] ||| [I]",
            spanr->GetHypothesis(0)->GetRuleString(str01, 'I')));
        ret = min(ret, CheckString("[I] ||| [F] [B]",
            span01->GetHypothesis(0)->GetRuleString(str01, 'F', 'B')));
        // Create a hypergraph
        HyperGraph hg;
        hg.SetStack(0, 0, span00);
        hg.SetStack(1, 1, span11);
        hg.SetStack(0, 1, span01);
        hg.SetStack(0, 2, spanr);
        // Check that the trimmed hypergraph only contains rules that should be there
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_FOR);
        span01->GetHypothesis(0)->SetLeftChild(NULL);
        span01->GetHypothesis(0)->SetRightChild(NULL);
        ostringstream graph_stream1;
        hg.PrintHyperGraph(str01, graph_stream1);
        ret = min(ret, CheckString("{\"rules\": [\"[F] ||| 0 1\", \"[R] ||| [F]\"], \"nodes\": [[{\"feature\":{\"parser\":1},\"rule\":1}], [{\"tail\":[0],\"feature\":{\"parser\":1},\"rule\":2}]], \"goal\": 1}",graph_stream1.str()));
        // Make another hypothesis
        span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_INV);
        span01->GetHypothesis(0)->SetLeftChild(span00);
        span01->GetHypothesis(0)->SetRightChild(span11);
        span01->AddHypothesis(new Hypothesis(1,1,0,0,1,-1,-1,HyperEdge::EDGE_STR,-1,1,-1,span00,span11));
        // Print again
        ostringstream graph_stream;
        hg.PrintHyperGraph(str01, graph_stream);
        ret = min(ret, CheckString("{\"rules\": [\"[F] ||| 0\", \"[B] ||| 1\", \"[I] ||| [F] [B]\", \"[S] ||| [F] [B]\", \"[R] ||| [I]\", \"[R] ||| [S]\"], \"nodes\": [[{\"feature\":{\"parser\":1},\"rule\":1}], [{\"feature\":{\"parser\":1},\"rule\":2}], [{\"tail\":[0,1],\"feature\":{\"parser\":1},\"rule\":3}], [{\"tail\":[0,1],\"feature\":{\"parser\":1},\"rule\":4}], [{\"tail\":[2],\"feature\":{\"parser\":1},\"rule\":5}, {\"tail\":[3],\"feature\":{\"parser\":1},\"rule\":6}]], \"goal\": 4}",graph_stream.str()));
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestGetTrgSpanID()" << endl; if(TestGetTrgSpanID()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestGetEdgeFeaturesAndWeights()" << endl; if(TestGetEdgeFeaturesAndWeights()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestGetNonLocalFeaturesAndWeights()" << endl; if(TestGetNonLocalFeaturesAndWeights()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestProcessOneSpan()" << endl; if(TestProcessOneSpan()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraph()" << endl; if(TestBuildHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphCubeGrowing()" << endl; if(TestBuildHyperGraphCubeGrowing()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphMultiThreads()" << endl; if(TestBuildHyperGraphMultiThreads()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphSaveFeatures()" << endl; if(TestBuildHyperGraphSaveFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateLoss()" << endl; if(TestAccumulateLoss()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateFeatures()" << endl; if(TestAccumulateFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestRescore()" << endl; if(TestRescore()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestReorderingAndPrint()" << endl; if(TestReorderingAndPrint()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestPrintHyperGraph()" << endl; if(TestPrintHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestHyperGraph Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge12t, edge12nt, edge02;
    CombinedAlign cal;
    Ranks ranks;
    FeatureDataSequence sent, sent_pos;
    ReordererModel model;
    std::vector<double> weights;
    FeatureSet set, non_local_set;
    vector<FeatureDataBase*> datas;
    FeatureSequence *featw, *featp;
    TargetSpan *ts00, *ts01, *ts11, *tsr;
    HyperGraph my_hg;


};

}

#endif
