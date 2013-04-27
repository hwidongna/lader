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
using namespace std;

namespace lader {

class TestDiscontinuousHyperGraph : public TestBase {

public:

    TestDiscontinuousHyperGraph() :
            edge00(0, -1, 0, HyperEdge::EDGE_FOR), 
            edge11(1, -1, 1, HyperEdge::EDGE_FOR), 
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge33(3, -1, 3, HyperEdge::EDGE_FOR),
            edge0_2b(0,0, 2,2, HyperEdge::EDGE_INV),
            edge1_3b(1,1, 3,3, HyperEdge::EDGE_INV),
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
        // Create a reorderer model with two weights
        model.SetWeight("W1", 1);
        model.SetWeight("W2", 2);
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
        my_hg = new DiscontinuousHyperGraph(1); // gap size 1
        // Make the target side spans
        ts00 = new TargetSpan(0,0,0,0);
        ts11 = new TargetSpan(1,1,1,1);
        ts22 = new TargetSpan(2,2,2,2);
        ts33 = new TargetSpan(3,3,3,3);
        ts0_2b = new DiscontinuousTargetSpan(0,0, 2,2, 2,0);
        ts1_3b = new DiscontinuousTargetSpan(1,1, 3,3, 3,1);
        ts01 = new TargetSpan(0,1,0,1);
        ts23 = new TargetSpan(2,3,2,3);
        ts03f = new TargetSpan(0,3,0,3);
        tsRoot = new TargetSpan(0,3,0,3);
        // Add the hypotheses
        ts00->AddHypothesis(Hypothesis(1,1, edge00.Clone(), 0,0));
        ts11->AddHypothesis(Hypothesis(2,2, edge11.Clone(), 1,1));
        ts22->AddHypothesis(Hypothesis(3,3, edge22.Clone(), 2,2));
        ts33->AddHypothesis(Hypothesis(4,4, edge33.Clone(), 3,3));
        ts0_2b->AddHypothesis(DiscontinuousHypothesis(5,1+3, edge0_2b.Clone(), 2,0, 0,0, ts00,ts22));
        ts1_3b->AddHypothesis(DiscontinuousHypothesis(6,2+4, edge1_3b.Clone(), 3,1, 0,0, ts11,ts33));
        ts01->AddHypothesis(DiscontinuousHypothesis(0,1, edge01.Clone(), 0,1));
        ts23->AddHypothesis(DiscontinuousHypothesis(2,3, edge23.Clone(), 2,3));
        // the viterbi score of combining ts0_2b and ts1_3b is greater than that of ts01 and ts23
        ts03f->AddHypothesis(Hypothesis(7,1+3+2+4+5+6, edge03f.Clone(), 0,3, 0,0, ts0_2b, ts1_3b));
        ts03f->AddHypothesis(Hypothesis(7,0+1+2+3, edge03f.Clone(), 0,3, 0,0, ts01, ts23));
        tsRoot->AddHypothesis(Hypothesis(0,1+3+2+4+5+6+7, 0,3, 0,3, HyperEdge::EDGE_ROOT,-1, 0,-1,ts03f));
        tsRoot->AddHypothesis(Hypothesis(0,0+1+2+3, 0,3, 0,3, HyperEdge::EDGE_ROOT,-1, 1,-1,ts03f));
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
        // Make the stacks
        SpanStack *stack00 = new SpanStack, *stack11 = new SpanStack,
        		*stack22 = new SpanStack, *stack33 = new SpanStack,
        		*stack0_2 = new SpanStack, *stack1_3 = new SpanStack,
        		*stack01 = new SpanStack, *stack23 = new SpanStack,
        		*stack03 = new SpanStack, *stackRoot = new SpanStack;
        stack00->AddSpan(ts00); stack11->AddSpan(ts11);
        stack22->AddSpan(ts22); stack33->AddSpan(ts33);
        stack0_2->AddSpan(ts0_2b); stack1_3->AddSpan(ts1_3b);
        stack01->AddSpan(ts01); stack23->AddSpan(ts23);
        stack03->AddSpan(ts03f); stackRoot->AddSpan(tsRoot);
        my_hg->HyperGraph::SetStack(0,0,stack00);
        my_hg->HyperGraph::SetStack(0,1,stack01);
        my_hg->HyperGraph::SetStack(1,1,stack11);
        my_hg->HyperGraph::SetStack(0,2,new SpanStack); // just for tesing
        my_hg->HyperGraph::SetStack(1,2,new SpanStack); // just for tesing
        my_hg->HyperGraph::SetStack(2,2,stack22);
        my_hg->HyperGraph::SetStack(0,3,stack03);
        my_hg->HyperGraph::SetStack(1,3,new SpanStack); // just for tesing
        my_hg->HyperGraph::SetStack(2,3,stack23);
        my_hg->HyperGraph::SetStack(3,3,stack33);
        my_hg->SetStack(0,0, 2,2, stack0_2);
        my_hg->SetStack(1,1, 3,3, stack1_3);
        my_hg->HyperGraph::SetStack(0,4,stackRoot);
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
        ts0_2b->GetHypothesis(0)->SetLoss(5);
        ts1_3b->GetHypothesis(0)->SetLoss(6);
        ts01->GetHypothesis(0)->SetLoss(3);
        ts23->GetHypothesis(0)->SetLoss(4);
        // the loss of combining ts0_2b and ts1_3b will be greater than that of ts01 and ts23
        ts03f->GetHypothesis(0)->SetLoss(7);
        ts03f->GetHypothesis(1)->SetLoss(7);
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
        edge02exp.push_back(MakePair(string("SW||he||rice"), 1));
        edge02exp.push_back(MakePair(string("SP||PRP||NN"), 1));
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
        // Check to make sure that the weights are Ok
        double weight_act = hyper_graph.GetEdgeScore(model, set,
                                                     datas, edge0_2b);
        if(weight_act != 3) {
            cerr << "Weight is not the expected 3: "<<weight_act<<endl;
            ret = 0;
        }
        return ret;
    }

    // Test the processing of a single span
    int TestProcessOneSpan() {
    	// allow gap size 1
        DiscontinuousHyperGraph graph(1);
        // Create two spans for 00, 11, 22, and 33
        SpanStack *stack0 = new SpanStack, *stack1 = new SpanStack;
        SpanStack *stack2 = new SpanStack, *stack3 = new SpanStack;
        stack0->push_back(new TargetSpan(0,0,0,0));
        (*stack0)[0]->AddHypothesis(Hypothesis(1,1,0,0,0,0,HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(0, 0, stack0);
        stack1->push_back(new TargetSpan(1, 1, 1, 1));
        (*stack1)[0]->AddHypothesis(Hypothesis(2, 2, 1, 1, 1, 1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(1, 1, stack1);
        stack2->push_back(new TargetSpan(2, 2, 2, 2));
        (*stack2)[0]->AddHypothesis(Hypothesis(4, 4, 2, 2, 2, 2, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(2, 2, stack2);
        stack3->push_back(new TargetSpan(3, 3, 3, 3));
        (*stack3)[0]->AddHypothesis(Hypothesis(8, 8, 3, 3, 3, 3, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(3, 3, stack3);
        // Try processing 02
        set.SetMaxTerm(0);
        SpanStack *stack01 = graph.ProcessOneSpan(model, set, datas, 0, 1);
        const SpanStack *stack0_2 = graph.GetStack(0,0, 2,2);
        // The stack should contain two target spans (2,0) and (0,2),
        // each with one hypothesis
        int ret = 1;
        if(stack0_2->size() != 2) {
            cerr << "stack0_2->size() != 2: " << stack0_2->size() << endl; ret = 0;
        } else if((*stack0_2)[0]->GetHypotheses().size() != 1) {
            cerr << "(*stack0_2)[0].size() != 1: " << (*stack0_2)[0]->GetHypotheses().size() << endl; ret = 0;
        } else if((*stack0_2)[1]->GetHypotheses().size() != 1) {
            cerr << "(*stack0_2)[1].size() != 1: " << (*stack0_2)[1]->GetHypotheses().size() << endl; ret = 0;
        }
        if(!ret) return 0;
        // Check to make sure that the scores are in order
        vector<double> score_exp(2,1+4), score_act(2);
        score_act[0] = (*stack0_2)[0]->GetHypothesis(0)->GetScore();
        score_act[1] = (*stack0_2)[1]->GetHypothesis(0)->GetScore();
        ret = CheckVector(score_exp, score_act);

        return ret;
    }

    // Test the processing of a single span
    int TestProcessOneSpanNoSave() {
    	DiscontinuousHyperGraph graph(1);
        // Create two spans for 00, 11, 22, and 33
        SpanStack *stack0 = new SpanStack, *stack1 = new SpanStack;
        SpanStack *stack2 = new SpanStack, *stack3 = new SpanStack;
        stack0->push_back(new TargetSpan(0,0, -1,-1));
        (*stack0)[0]->AddHypothesis(Hypothesis(1,1,0,0,-1,-1,HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(0, 0, stack0);
        stack1->push_back(new TargetSpan(1,1, -1,-1));
        (*stack1)[0]->AddHypothesis(Hypothesis(2, 2, 1, 1, -1, -1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(1, 1, stack1);
        stack2->push_back(new TargetSpan(2,2, -1,-1));
        (*stack2)[0]->AddHypothesis(Hypothesis(4, 4, 2, 2, -1, -1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(2, 2, stack2);
        stack3->push_back(new TargetSpan(3, 3, -1,-1));
        (*stack3)[0]->AddHypothesis(Hypothesis(8, 8, 3, 3, -1, -1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(3, 3, stack3);
        // Try processing 01
        set.SetMaxTerm(0);
        SpanStack *stack01 = graph.ProcessOneSpan(model, set, datas, 0, 1, 0, false);
        // The stack should contain two target spans (1,0) and (0,1),
        // each with two hypotheses
        int ret = 1;
        if(stack01->size() != 1) {
            cerr << "stack01->size() != 1: " << stack01->size() << endl; ret = 0;
            for(int i = 0; i < (int)stack01->size(); i++)
                cerr << " " << i << ": " << (*stack01)[i]->GetTrgLeft() << ", " <<(*stack01)[i]->GetTrgRight() << endl;
        } else if((*stack01)[0]->GetHypotheses().size() != 4) {
            cerr << "(*stack01)[0].size() != 4: " << (*stack01)[0]->GetHypotheses().size() << endl; ret = 0;
        }
        if(!ret) return 0;
        // Check to make sure that the scores are in order
        vector<double> score_exp(4,0), score_act(4);
        score_exp[0] = 3; score_exp[1] = 3;
        score_act[0] = (*stack01)[0]->GetHypothesis(0)->GetScore();
        score_act[1] = (*stack01)[0]->GetHypothesis(1)->GetScore();
        score_act[2] = (*stack01)[0]->GetHypothesis(2)->GetScore();
        score_act[3] = (*stack01)[0]->GetHypothesis(3)->GetScore();
        ret = CheckVector(score_exp, score_act);
        // Check to make sure that pruning works
        set.SetMaxTerm(0);
        SpanStack *stack01pruned = graph.ProcessOneSpan(model, set, datas, 0, 1, 3, false);
        if(stack01pruned->size() != 1) {
            cerr << "stack01pruned->size() != 1: " << stack01pruned->size() << endl; ret = 0;
        } else if((*stack01pruned)[0]->GetHypotheses().size() != 3) {
            cerr << "(*stack01pruned)[0].size() != 3: " << (*stack01pruned)[0]->GetHypotheses().size() << endl; ret = 0;
        }
        return ret;
    }

    int TestBuildHyperGraph() {
        HyperGraph graph = DiscontinuousHyperGraph(1);
        set.SetMaxTerm(0);
        graph.BuildHyperGraph(model, set, datas);
        const std::vector<SpanStack*> & stacks = graph.GetStacks();
        int ret = 1;
        SpanStack * stack03 = graph.HyperGraph::GetStack(0, 3);
        SpanStack * stackRoot = graph.HyperGraph::GetStack(-1, 3);
        // The total number of stacks should be 11: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 root
        if(stacks.size() != 11) {
            cerr << "stacks.size() != 11: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 12: 0-1 0-2 0-3 1-0 1-2 1-3 2-0 2-1 2-3 3-0 3-1 3-2
        } else if (stack03->size() != 12) {
            cerr << "Root node stack03->size() != 12: " << stack03->size()<< endl;
            BOOST_FOREACH(const TargetSpan *span, stack03->GetSpans())
                cerr << " " << span->GetTrgLeft() << "-" <<span->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->GetSpans().size() != stack03->size()) {
            cerr << "Root hypotheses " << stackRoot->GetSpans().size()
                 << " and root spans " << stack03->size() << " don't match." << endl; ret = 0;
        }
        return ret;
    }

    int TestBuildHyperGraphGap2() {
        HyperGraph graph = DiscontinuousHyperGraph(2);
        set.SetMaxTerm(0);
        sent.FromString("this sentence has five words");
		sent_pos.FromString("PRP NNS VB ADJ NNP");
        graph.BuildHyperGraph(model, set, datas);
        const std::vector<SpanStack*> & stacks = graph.GetStacks();
        int ret = 1;
        SpanStack * stack04 = graph.HyperGraph::GetStack(0, 4);
        SpanStack * stackRoot = graph.HyperGraph::GetStack(-1, 4);
        // The total number of stacks should be 6*5/2 + 1: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 0-4 1-4 2-4 3-4 4-4 root
        if(stacks.size() != 16) {
            cerr << "stacks.size() != 16: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 5*4:
        } else if (stack04->size() != 20) {
            cerr << "Root node stack04->size() != 20: " << stack04->size()<< endl;
            BOOST_FOREACH(const TargetSpan *span, stack04->GetSpans())
                cerr << " " << span->GetTrgLeft() << "-" <<span->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->GetSpans().size() != stack04->size()) {
            cerr << "Root hypotheses " << stackRoot->GetSpans().size()
                 << " and root spans " << stack04->size() << " don't match." << endl; ret = 0;
        }
        return ret;
    }


    int TestBuildHyperGraphNoSave() {
    	HyperGraph graph = DiscontinuousHyperGraph(1);
        set.SetMaxTerm(0);
        graph.BuildHyperGraph(model, set, datas, 0, false);
        const std::vector<SpanStack*> & stacks = graph.GetStacks();
        int ret = 1;
        SpanStack * stack03 = graph.HyperGraph::GetStack(0, 3);
        SpanStack * stackRoot = graph.HyperGraph::GetStack(-1, 3);
        // The total number of stacks should be 11: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 root
        if(stacks.size() != 11) {
            cerr << "stacks.size() != 11: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 1: -1-1
        } else if (stack03->size() != 1) {
            cerr << "Root node stack03->size() != 1: " << stack03->size()<< endl;
            BOOST_FOREACH(const TargetSpan *span, stack03->GetSpans())
                cerr << " " << span->GetTrgLeft() << "-" <<span->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->GetSpans().size() != stack03->size()) {
            cerr << "Root hypotheses " << stackRoot->GetSpans().size()
                 << " and root spans " << stack03->size() << " don't match." << endl; ret = 0;
        }
        return ret;
    }

    int TestAccumulateLoss() {
        // The value of the loss should be 1+2+3+4+5+6+7 = 28 (01:1 and 23:5 are not in the best)
        double val = my_hg->AccumulateLoss(ts03f);
        int ret = 1;
        if(val != 28) {
            cerr << "my_hg->AccumulateLoss() != 28: " <<
                     my_hg->AccumulateLoss(ts03f) << endl; ret = 0;
        }
        // Test the rescoring
        return ret;
    }

    int TestAccumulateFeatures() {
        FeatureVectorInt act = my_hg->AccumulateFeatures(ts03f);
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
        // Rescoring with loss +1 should pick the inverted terminal
        // with a loss of 1+2+3+4+5+6+7, minus a weight of 1*7 -> 21
        score = my_hg->Rescore(mod, 1.0);
        if(score != 21) {
            cerr << "Rescore(mod, 1.0) != 21: " << score << endl; ret = 0;
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
        TargetSpan *span00 = new TargetSpan(0,0,-1,-1),
                   *span01 = new TargetSpan(0,1,-1,-1),
                   *span11 = new TargetSpan(1,1,-1,-1),
                   *spanr = new TargetSpan(0,1,-1,-1);
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
        TargetSpan *span00 = new TargetSpan(0,0,-1,-1),
                   *span01 = new TargetSpan(0,1,-1,-1),
                   *span11 = new TargetSpan(1,1,-1,-1),
                   *spanr = new TargetSpan(0,1,-1,-1);
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
        SpanStack * stack00 = new SpanStack; stack00->push_back(span00); hg.SetStack(0, 0, stack00);
        SpanStack * stack11 = new SpanStack; stack11->push_back(span11); hg.SetStack(1, 1, stack11);
        SpanStack * stack01 = new SpanStack; stack01->push_back(span01); hg.SetStack(0, 1, stack01);
        SpanStack * stackr  = new SpanStack; stackr->push_back(spanr); hg.SetStack(0, 2, stackr);
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
        done++; cout << "TestProcessOneSpanNoSave()" << endl; if(TestProcessOneSpanNoSave()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraph()" << endl; if(TestBuildHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphNoSave()" << endl; if(TestBuildHyperGraphNoSave()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateLoss()" << endl; if(TestAccumulateLoss()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAccumulateFeatures()" << endl; if(TestAccumulateFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestRescore()" << endl; if(TestRescore()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestReorderingAndPrint()" << endl; if(TestReorderingAndPrint()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestPrintHyperGraph()" << endl; if(TestPrintHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphGap2()" << endl; if(TestBuildHyperGraphGap2()) succeeded++; else cout << "FAILED!!!" << endl;
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
    TargetSpan *ts00, *ts11, *ts22, *ts33, *ts03f, *ts01, *ts23, *tsRoot;
    DiscontinuousTargetSpan *ts1_3b, *ts0_2b;
    DiscontinuousHyperGraph *my_hg;


};

}

#endif
