#ifndef TEST_DISCONTINUOUS_HYPER_GRAPH_H__
#define TEST_DISCONTINUOUS_HYPER_GRAPH_H__

#include "test-base.h"
#include <climits>
#include <lader/hyper-graph.h>
#include <lader/discontinuous-hyper-edge.h>
#include <lader/discontinuous-target-span.h>
#include <lader/alignment.h>
#include <lader/reorderer-model.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-sequence.h>
#include <lader/feature-set.h>
#include <lader/ranks.h>
#include <lader/feature-base.h>
#include <lader/discontinuous-hyper-graph.h>
using namespace std;

namespace lader {

class TestDiscontinuousHyperGraph : public TestBase {

public:

    TestDiscontinuousHyperGraph() :
            edge00(0, -1, 0, HyperEdge::EDGE_FOR), 
            edge11(1, -1, 1, HyperEdge::EDGE_FOR), 
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge33(3, -1, 3, HyperEdge::EDGE_FOR),
            edge0_2b(0,0, -1, 2,2, HyperEdge::EDGE_INV),
            edge1_3b(1,1, -1, 3,3, HyperEdge::EDGE_INV),
            edge01(0, -1, 1, HyperEdge::EDGE_FOR),
            edge23(2, -1, 3, HyperEdge::EDGE_FOR),
            edge03f(0, 2, 3, HyperEdge::EDGE_STR)
            {
        // Create a combined Inside-Out alignment
        //  .x..
        //  ...x
        //  x...
    	//  ..x.
        vector<string> words(4,"x");
        Alignment al(MakePair(4,4));
        al.AddAlignment(MakePair(0,1));
        al.AddAlignment(MakePair(1,3));
        al.AddAlignment(MakePair(2,0));
        al.AddAlignment(MakePair(3,2));
        cal = CombinedAlign(words, al);
        // Create a sentence
        string str = "he ate rice .";
        sent.FromString(str);
        string str_pos = "PRP VBD NN P";
        sent_pos.FromString(str_pos);
        // Create a reorderer model with weights
        model.SetWeight("SW||ate||rice||IC", -1);
        model.SetWeight("SP||VBD||NN||IC", -1);
        // prefer non-BTG more than BTG
        model.SetWeight("SW||he||rice||ID", 1);
        model.SetWeight("SP||PRP||NN||ID", 1);
        model.SetWeight("SW||he||rice||SD", 2);
        model.SetWeight("SP||PRP||NN||SD", 2);
        model.SetWeight("SW||ate||.||SD", 3);
        model.SetWeight("SP||VBD||P||SD", 3);
        // prefer the last straight
        model.SetWeight("SW||he||.||SC", 3);
        model.SetWeight("SP||he||.||IC", -3);
        // Set up the feature generators
        featw = new FeatureSequence;
        featp = new FeatureSequence;
        featw->ParseConfiguration("SW%LL%RR%ET");
        featp->ParseConfiguration("SP%LL%RR%ET");
        // Set up the feature set
        set.AddFeatureGenerator(featw);
        set.AddFeatureGenerator(featp);
        // Set up the data
        datas.push_back(&sent);
        datas.push_back(&sent_pos);

        // ------ Make a hypergraph for testing various things ------
        my_hg = new DiscontinuousHyperGraph(1); // gap size 1
        // Make the target side spans
        ts00 = new TargetSpan(0,0);
        ts11 = new TargetSpan(1,1);
        ts22 = new TargetSpan(2,2);
        ts33 = new TargetSpan(3,3);
        ts0_2 = new DiscontinuousTargetSpan(0,0, 2,2);
        ts1_3 = new DiscontinuousTargetSpan(1,1, 3,3);
        ts01 = new TargetSpan(0,1);
        ts23 = new TargetSpan(2,3);
        ts03 = new TargetSpan(0,3);
        tsRoot = new TargetSpan(0,3);
        // Add the hypotheses
        ts00->AddHypothesis(Hypothesis(1,1, edge00.Clone(), 0,0));
        ts11->AddHypothesis(Hypothesis(2,2, edge11.Clone(), 1,1));
        ts22->AddHypothesis(Hypothesis(3,3, edge22.Clone(), 2,2));
        ts33->AddHypothesis(Hypothesis(4,4, edge33.Clone(), 3,3));
        ts0_2->AddHypothesis(DiscontinuousHypothesis(1+3+5,5, edge0_2b.Clone(), 2,0, 0,0, ts00,ts22));
        ts1_3->AddHypothesis(DiscontinuousHypothesis(2+4+6,6, edge1_3b.Clone(), 3,1, 0,0, ts11,ts33));
        ts01->AddHypothesis(DiscontinuousHypothesis(1,1, edge01.Clone(), 0,1));
        ts23->AddHypothesis(DiscontinuousHypothesis(2,2, edge23.Clone(), 2,3));
        // the viterbi score of combining ts0_2b and ts1_3b is greater than that of ts01 and ts23
        ts03->AddHypothesis(Hypothesis(1+3+5+2+4+6+7,7, edge03f.Clone(), 0,3, 0,0, ts0_2, ts1_3));
        ts03->AddHypothesis(Hypothesis(1+2+7,7, edge03f.Clone(), 0,3, 0,0, ts01, ts23));
        tsRoot->AddHypothesis(Hypothesis(1+3+5+2+4+6+7, 0, 0,3, 0,3, HyperEdge::EDGE_ROOT,-1, 0,-1,ts03));
        tsRoot->AddHypothesis(Hypothesis(1+2+7, 0, 0,3, 0,3, HyperEdge::EDGE_ROOT,-1, 1,-1,ts03));
        // Add the features
        FeatureVectorInt 
            *fv00 = new FeatureVectorInt(1, MakePair(1,1)),
            *fv11 = new FeatureVectorInt(1, MakePair(2,1)),
            *fv22 = new FeatureVectorInt(1, MakePair(3,1)),
            *fv33 = new FeatureVectorInt(1, MakePair(4,1)),
            *fv0_2b = new FeatureVectorInt(1,MakePair(5,1)),
            *fv1_3b = new FeatureVectorInt(1,MakePair(6,1)),
            *fv01 = new FeatureVectorInt(1,MakePair(5,1)),
        	*fv23 = new FeatureVectorInt(1,MakePair(6,1)),
        	*fv03f = new FeatureVectorInt(1,MakePair(7,1));
        fv00->push_back(MakePair(10,1));
        fv11->push_back(MakePair(10,1));
        fv22->push_back(MakePair(10,1));
        fv33->push_back(MakePair(10,1));
        fv0_2b->push_back(MakePair(10,1));
        fv1_3b->push_back(MakePair(10,1));
        fv01->push_back(MakePair(10,1));
        fv23->push_back(MakePair(10,1));
        fv03f->push_back(MakePair(10,1));
        my_hg->SetEdgeFeatures(HyperEdge(0,-1,0,HyperEdge::EDGE_FOR), fv00);
        my_hg->SetEdgeFeatures(HyperEdge(1,-1,1,HyperEdge::EDGE_FOR), fv11);
        my_hg->SetEdgeFeatures(HyperEdge(2,-1,2,HyperEdge::EDGE_FOR), fv22);
        my_hg->SetEdgeFeatures(HyperEdge(3,-1,3,HyperEdge::EDGE_FOR), fv33);
        my_hg->SetEdgeFeatures(edge0_2b, fv0_2b);
        my_hg->SetEdgeFeatures(edge1_3b, fv1_3b);
        my_hg->SetEdgeFeatures(edge01, fv01);
        my_hg->SetEdgeFeatures(edge23, fv23);
        my_hg->SetEdgeFeatures(edge03f, fv03f);
        // Set the stacks
        my_hg->HyperGraph::SetStack(0,0,ts00);
        my_hg->HyperGraph::SetStack(0,1,ts01);
        my_hg->HyperGraph::SetStack(1,1,ts11);
        my_hg->HyperGraph::SetStack(0,2,new TargetSpan(0,2)); // just for tesing
        my_hg->HyperGraph::SetStack(1,2,new TargetSpan(1,2)); // just for tesing
        my_hg->HyperGraph::SetStack(2,2,ts22);
        my_hg->HyperGraph::SetStack(0,3,ts03);
        my_hg->HyperGraph::SetStack(1,3,new TargetSpan(1,3)); // just for tesing
        my_hg->HyperGraph::SetStack(2,3,ts23);
        my_hg->HyperGraph::SetStack(3,3,ts33);
        my_hg->SetStack(0,0, 2,2, ts0_2);
        my_hg->SetStack(1,1, 3,3, ts1_3);
        my_hg->HyperGraph::SetStack(0,4,tsRoot);
//        BOOST_FOREACH(SpanStack * stack, my_hg->GetStacks())
//			BOOST_FOREACH(TargetSpan * trg, stack->GetSpans()){
//        		cerr << "Target span " << *trg << endl;
//				BOOST_FOREACH(Hypothesis * hyp, trg->GetHypotheses())
//					cerr << "Hypothesis " << *hyp << endl;
//        	}
        // Add the loss
        ts00->GetHypothesis(0)->SetLoss(1);
        ts11->GetHypothesis(0)->SetLoss(2);
        ts22->GetHypothesis(0)->SetLoss(3);
        ts33->GetHypothesis(0)->SetLoss(4);
        ts0_2->GetHypothesis(0)->SetLoss(5);
        ts1_3->GetHypothesis(0)->SetLoss(6);
        ts01->GetHypothesis(0)->SetLoss(3);
        ts23->GetHypothesis(0)->SetLoss(4);
        // the loss of combining ts0_2 and ts1_3 will be greater than that of ts01 and ts23
        ts03->GetHypothesis(0)->SetLoss(7);
        ts03->GetHypothesis(1)->SetLoss(7);
        // // Sort the stacks so we get the best value first
        // BOOST_FOREACH(SpanStack & stack, my_hg->GetStacks())
        //     stack.SortSpans(true);
    }

    virtual ~TestDiscontinuousHyperGraph(){
    	delete my_hg;
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
        edge02exp.push_back(MakePair(string("SW||he||rice||ID"), 1));
        edge02exp.push_back(MakePair(string("SP||PRP||NN||ID"), 1));
        FeatureVectorInt edge02intexp;
        edge02intexp.push_back(MakePair(0, 1));
        edge02intexp.push_back(MakePair(1, 1));
        // Make the hypergraph and get the features
        DiscontinuousHyperGraph hyper_graph(1);
        // Generate the features
        const FeatureVectorInt * edge02int =
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge0_2b);
        FeatureVectorString * edge02act =
                        mod.StringifyFeatureVector(*edge02int);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(edge02exp, *edge02act);
        ret *= CheckVector(edge02intexp, *edge02int);
        // Generate the features again
        const FeatureVectorInt * edge02int2 =
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge0_2b);
        // Make sure that the pointers are equal
        if(edge02int != edge02int2) {
            cerr << "Edge pointers are not equal." << endl;
            ret = 0;
        }
        mod.SetWeight("SW||he||rice||ID", 1);
        mod.SetWeight("SP||PRP||NN||ID", 2);
        // Check to make sure that the weights are Ok
        double weight_act = hyper_graph.GetEdgeScore(mod, set,
                                                     datas, edge0_2b);
        if(weight_act != 1+2) {
            cerr << "Weight is not the expected 3: "<<weight_act<<endl;
            ret = 0;
        }
        return ret;
    }

    // Test the processing of a single span
    int TestProcessOneSpan() {
    	// allow gap size 1
        DiscontinuousHyperGraph graph(1);
        // Create spans for 0, 1, 2, and 3
        TargetSpan *stack0 = new TargetSpan(0,0);
        TargetSpan *stack1 = new TargetSpan(1,1);
        TargetSpan *stack2 = new TargetSpan(2,2);
		TargetSpan *stack3 = new TargetSpan(3,3);
        stack0->AddHypothesis(Hypothesis(1,1, 0,0, 0,0,HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(0, 0, stack0);
        stack1->AddHypothesis(Hypothesis(2,2, 1,1, 1,1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(1, 1, stack1);
        stack2->AddHypothesis(Hypothesis(4,4, 2,2, 2,2, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(2, 2, stack2);
        stack3->AddHypothesis(Hypothesis(8,8, 3,3, 3,3, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(3, 3, stack3);
        // Try processing stack01, which lead to set stack0_2
        set.SetMaxTerm(0);
        graph.HyperGraph::SetStack(0, 1, graph.ProcessOneSpan(model, set, datas, 0, 1));
        const TargetSpan *stack0_2 = graph.GetStack(0,0, 2,2);
        // The stack should contain two target spans (2,0) and (0,2),
        // each with one hypothesis
        int ret = 1;
        if(stack0_2->size() != 2) {
            cerr << "stack0_2->size() != 2: " << stack0_2->size() << endl; ret = 0;
        }
        if(!ret) return 0;
        // Check to make sure that the scores are in order
        vector<double> score_exp(2), score_act(2);
        // (0,2) has EdgeFeatureScore 2+2, while (2,0) has 1+1
        score_exp[0] = 1+4+4; score_exp[1] = 1+4+2;
        score_act[0] = stack0_2->GetHypothesis(0)->GetScore();
        score_act[1] = stack0_2->GetHypothesis(1)->GetScore();
        ret = CheckVector(score_exp, score_act);

        return ret;
    }

    int TestBuildHyperGraph() {
        DiscontinuousHyperGraph graph(1);
        set.SetMaxTerm(1);
        graph.BuildHyperGraph(model, set, datas, 4*3*2);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        TargetSpan * stack03 = graph.HyperGraph::GetStack(0, 3);
        TargetSpan * stackRoot = graph.HyperGraph::GetRoot();
        // The total number of stacks should be 11: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 root
        if(stacks.size() != 11) {
            cerr << "stacks.size() != 11: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 4*3*2: including non-BTGs
        } else if (stack03->size() != 4*3*2) {
            cerr << "Root node stack03->size() != 4*3*2: " << stack03->size()<< endl;
            BOOST_FOREACH(const Hypothesis *hyp, stack03->GetHypotheses())
            	cerr << " " << hyp->GetTrgLeft() << "-" <<hyp->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->size() != stack03->size()) {
            cerr << "Root hypotheses " << stackRoot->size()
                 << " and root spans " << stack03->size() << " don't match." << endl; ret = 0;
        }

        BOOST_FOREACH(TargetSpan * stack, stacks)
			BOOST_FOREACH(Hypothesis * hyp, stack->GetHypotheses()){
				Hypothesis * lhyp = hyp->GetLeftHyp();
				Hypothesis * rhyp = hyp->GetRightHyp();
				if (hyp->GetScore() !=
						hyp->GetSingleScore()
						+ (lhyp ? lhyp->GetScore() : 0)
						+ (rhyp ? rhyp->GetScore() : 0) ){
					cerr << "Incorrect viterbi score " << *hyp << endl;
					hyp->PrintChildren(cerr);
					ret = 0;
				}
			}
        return ret;
    }

    int TestBuildHyperGraphGap2() {
        DiscontinuousHyperGraph graph(2);
        set.SetMaxTerm(0);
        sent.FromString("this sentence has five words");
		sent_pos.FromString("PRP NNS VB ADJ NNP");
        graph.BuildHyperGraph(model, set, datas, 100);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        TargetSpan * stack04 = graph.HyperGraph::GetStack(0, 4);
        TargetSpan * stackRoot = graph.HyperGraph::GetStack(-1, 4);
        // The total number of stacks should be 6*5/2 + 1: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 0-4 1-4 2-4 3-4 4-4 root
        if(stacks.size() != 16) {
            cerr << "stacks.size() != 16: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 100:
        } else if (stack04->size() != 100) {
            cerr << "Root node stack04->size() != 100: " << stack04->size()<< endl;
            BOOST_FOREACH(const Hypothesis *hyp, stack04->GetHypotheses())
            	cerr << " " << hyp->GetTrgLeft() << "-" <<hyp->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->size() != stack04->size()) {
            cerr << "Root hypotheses " << stackRoot->size()
                 << " and root spans " << stack04->size() << " don't match." << endl; ret = 0;
        }
        return ret;
    }

    int TestBuildHyperGraphSize1() {
		DiscontinuousHyperGraph graph(0);
		set.SetMaxTerm(0);
		sent.FromString("one");
		sent_pos.FromString("NNS");
		graph.BuildHyperGraph(model, set, datas);
		const std::vector<TargetSpan*> & stacks = graph.GetStacks();
		int ret = 1;
		// The total number of stacks should be 1 + 1: 0-0 root
		if(stacks.size() != 2) {
			cerr << "stacks.size() != 2: " << stacks.size() << endl; ret = 0;
		}
		return ret;
	}

    int TestAccumulateLoss() {
        // The value of the loss should be 1+2+3+4+5+6+7 = 28 (01:1 and 23:5 are not in the best)
        double val = ts03->GetHypothesis(0)->AccumulateLoss();
        int ret = 1;
        if(val != 28) {
            cerr << "ts03->AccumulateLoss() != 28: " <<
                     ts03->GetHypothesis(0)->AccumulateLoss() << endl; ret = 0;
        }
        // Test the rescoring
        return ret;
    }

    int TestAccumulateFeatures() {
    	FeatureVectorInt act = my_hg->GetBest()->AccumulateFeatures(my_hg->GetFeatures());
        FeatureVectorInt exp;
        exp.push_back(MakePair(1,1));
        exp.push_back(MakePair(2,1));
        exp.push_back(MakePair(3,1));
        exp.push_back(MakePair(4,1));
        exp.push_back(MakePair(5,1));
        exp.push_back(MakePair(6,1));
        exp.push_back(MakePair(7,1));
        exp.push_back(MakePair(10,1*7));
        // Test the rescoring
        return CheckVector(exp, act);
    }

    // Test that rescoring works
    int TestRescore() {
        // Create a model that assigns a weight of -1 to each production
        ReordererModel mod;
        for(int i = 0; i < 20; i++) {
            ostringstream oss;
            oss << "WEIGHT" << i;
            mod.SetWeight(oss.str(), 0);
        }
        mod.SetWeight("WEIGHT10", -1);
        int ret = 1;
        // Simply rescoring with this model should pick the forward production
        // with a score of -3
        double score = my_hg->Rescore(mod, 0.0);
        if(score != -3) {
            cerr << "Rescore(mod, 0.0) != -3: " << score << endl; ret = 0;
        }
        // Rescoring with loss +1 should pick the non-BTG
        // with a loss of 1+2+3+4+5+6+7, minus a weight of 1*7 -> 21
        score = my_hg->Rescore(mod, 1.0);
        if(score != 21) {
            cerr << "Rescore(mod, 1.0) != 21: " << score << endl; ret = 0;
        }
        // Rescoring with loss -1 should pick the BTG
		// with a loss of -1-2-7, minus a weight of 1*7 -> 21
		score = my_hg->Rescore(mod, -1);
		if(score != -17) {
			cerr << "Rescore(mod, -1) != -17: " << score << endl; ret = 0;
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
        span00->AddHypothesis(Hypothesis(1,1,0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(Hypothesis(1,1,1,1,-1,-1,HyperEdge::EDGE_FOR));
        span01->AddHypothesis(Hypothesis(1,1,0,1,-1,-1,HyperEdge::EDGE_FOR));
        spanr->AddHypothesis(Hypothesis(1,1,0,1,-1,-1,HyperEdge::EDGE_ROOT,-1,0,-1,span01));
        // Get the reordering for forward
        int ret = 1;
        vector<int> for_reorder; spanr->GetReordering(for_reorder);
         ostringstream for_oss; spanr->PrintParse(str01, for_oss);
         ret = min(ret, CheckVector(vec01, for_reorder));
         ret = min(ret, CheckString("(F (FW 0) (FW 1))", for_oss.str()));
         // Get the reordering bac backward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_BAC);
         vector<int> bac_reorder; spanr->GetReordering(bac_reorder);
         ostringstream bac_oss; spanr->PrintParse(str01, bac_oss);
         ret = min(ret, CheckVector(vec10, bac_reorder));
         ret = min(ret, CheckString("(B (BW 0) (BW 1))", bac_oss.str()));
         // Get the reordering for forward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_STR);
         span01->GetHypothesis(0)->SetLeftChild(span00);
         span01->GetHypothesis(0)->SetRightChild(span11);
         vector<int> str_reorder; spanr->GetReordering(str_reorder);
         ostringstream str_oss; spanr->PrintParse(str01, str_oss);
         ret = min(ret, CheckVector(vec01, str_reorder));
         ret = min(ret,CheckString("(S (F (FW 0)) (F (FW 1)))",str_oss.str()));
         // Get the reordering for forward
         span01->GetHypothesis(0)->SetType(HyperEdge::EDGE_INV);
         vector<int> inv_reorder; spanr->GetReordering(inv_reorder);
         ostringstream inv_oss; spanr->PrintParse(str01, inv_oss);
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
        span00->AddHypothesis(Hypothesis(1,1,0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(Hypothesis(1,1,1,1,-1,-1,HyperEdge::EDGE_BAC));
        span01->AddHypothesis(Hypothesis(1,1,0,1,-1,-1,HyperEdge::EDGE_INV,-1,1,-1,span00,span11));
        spanr->AddHypothesis(Hypothesis(1,1,0,1,-1,-1,HyperEdge::EDGE_ROOT,-1,0,-1,span01));
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
        span01->AddHypothesis(Hypothesis(1,1,0,1,-1,-1,HyperEdge::EDGE_STR,-1,1,-1,span00,span11));
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
        done++; cout << "TestProcessOneSpan()" << endl; if(TestProcessOneSpan()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraph()" << endl; if(TestBuildHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateLoss()" << endl; if(TestAccumulateLoss()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateFeatures()" << endl; if(TestAccumulateFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestRescore()" << endl; if(TestRescore()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestReorderingAndPrint()" << endl; if(TestReorderingAndPrint()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestPrintHyperGraph()" << endl; if(TestPrintHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphGap2()" << endl; if(TestBuildHyperGraphGap2()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphSize1()" << endl; if(TestBuildHyperGraphSize1()) succeeded++; else cout << "FAILED!!!" << endl;
        cout << "#### TestDiscontinuousHyperGraph Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge33, edge01, edge23, edge03f;
    DiscontinuousHyperEdge edge0_2b, edge1_3b;
    CombinedAlign cal;
    Ranks ranks;
    FeatureDataSequence sent, sent_pos;
    ReordererModel model;
    std::vector<double> weights;
    FeatureSet set;
    vector<FeatureDataBase*> datas;
    FeatureSequence *featw, *featp;
    TargetSpan *ts00, *ts11, *ts22, *ts33, *ts03, *ts01, *ts23, *tsRoot;
    DiscontinuousTargetSpan *ts1_3, *ts0_2;
    DiscontinuousHyperGraph *my_hg;


};

}

#endif
