#ifndef TEST_DISCONTINUOUS_HYPER_GRAPH_H__
#define TEST_DISCONTINUOUS_HYPER_GRAPH_H__

#include "test-base.h"
#include <climits>
#include <lader/hypothesis.h>
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
#include <lader/discontinuous-hypothesis.h>
#include "test-hyper-graph.h"

using namespace std;

namespace lader {

class TestDiscontinuousHyperGraph : public TestBase {

public:

    TestDiscontinuousHyperGraph() :
            edge00(0, -1, 0, HyperEdge::EDGE_FOR), 
            edge11(1, -1, 1, HyperEdge::EDGE_FOR), 
            edge22(2, -1, 2, HyperEdge::EDGE_FOR),
            edge33(3, -1, 3, HyperEdge::EDGE_FOR),
            edge0_2(0,0, -1, 2,2, HyperEdge::EDGE_INV),
            edge1_3(1,1, -1, 3,3, HyperEdge::EDGE_INV),
            edge01(0, -1, 1, HyperEdge::EDGE_FOR),
            edge23(2, -1, 3, HyperEdge::EDGE_FOR),
            edge03sd(0,1, 2, 3,3, HyperEdge::EDGE_STR),
            edge03sc(0, 2, 3, HyperEdge::EDGE_STR)
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
        ranks = Ranks(cal);
        // Create a sentence
        string str = "he ate rice .";
        sent.FromString(str);
        string str_pos = "PRP VBD NN P";
        sent_pos.FromString(str_pos);
        // Create a reorderer model with weights
        model.SetWeight("SW||ate||rice||IC", -1);
        model.SetWeight("SP||VBD||NN||IC", -1);
        // prefer non-ITG more than ITG
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
        ts03d = new DiscontinuousTargetSpan(0,1,3,3); // only for saving feature
        tsRoot = new TargetSpan(0,3);
        // Add the hypotheses
        ts00->AddHypothesis(new Hypothesis(1,-1, edge00.Clone(), 0,0));
        ts11->AddHypothesis(new Hypothesis(2,-1, edge11.Clone(), 1,1));
        ts22->AddHypothesis(new Hypothesis(3,-1, edge22.Clone(), 2,2));
        ts33->AddHypothesis(new Hypothesis(4,-1, edge33.Clone(), 3,3));
        ts0_2->AddHypothesis(new DiscontinuousHypothesis(1+3+5,-1, edge0_2.Clone(), 2,0, 0,0, ts00,ts22));
        ts1_3->AddHypothesis(new DiscontinuousHypothesis(2+4+6,-1, edge1_3.Clone(), 3,1, 0,0, ts11,ts33));
        ts01->AddHypothesis(new DiscontinuousHypothesis(1,-1, edge01.Clone(), 0,1));
        ts23->AddHypothesis(new DiscontinuousHypothesis(2,-1, edge23.Clone(), 2,3));
        // the viterbi score of combining ts0_2b and ts1_3b is greater than that of ts01 and ts23
        ts03->AddHypothesis(new Hypothesis(1+3+5+2+4+6+7,-1, edge03sd.Clone(), 0,3, 0,0, ts0_2, ts1_3));
        ts03->AddHypothesis(new Hypothesis(1+2+7,-1, edge03sc.Clone(), 0,3, 0,0, ts01, ts23));
        tsRoot->AddHypothesis(new Hypothesis(1+3+5+2+4+6+7, 0, 0,3, 0,3,  HyperEdge::EDGE_ROOT,-1, 0,-1,ts03));
        tsRoot->AddHypothesis(new Hypothesis(1+2+7, 0, 0,3, 0,3,  HyperEdge::EDGE_ROOT,-1, 1,-1,ts03));
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
        	*fv03sc = new FeatureVectorInt(1,MakePair(7,1)),
        	*fv03sd = new FeatureVectorInt(1,MakePair(8,1));
        fv00->push_back(MakePair(10,1));
        fv11->push_back(MakePair(10,1));
        fv22->push_back(MakePair(10,1));
        fv33->push_back(MakePair(10,1));
        fv0_2b->push_back(MakePair(10,1));
        fv1_3b->push_back(MakePair(10,1));
        fv01->push_back(MakePair(10,1));
        fv23->push_back(MakePair(10,1));
        fv03sc->push_back(MakePair(10,1));
        fv03sd->push_back(MakePair(10,1));
        ts00->SaveStraightFeautures(-1, fv00);
		ts11->SaveStraightFeautures(-1, fv11);
		ts22->SaveStraightFeautures(-1, fv22);
		ts33->SaveStraightFeautures(-1, fv33);
		ts0_2->SaveInvertedFeautures(-1, fv0_2b);
		ts1_3->SaveInvertedFeautures(-1, fv1_3b);
		ts01->SaveStraightFeautures(-1, fv01);
		ts23->SaveStraightFeautures(-1, fv23);
		ts03->SaveStraightFeautures(2, fv03sc);
		ts03d->SaveStraightFeautures(2, fv03sd); // only for saving feature
//        my_hg->SetEdgeFeatures(HyperEdge(0,-1,0,HyperEdge::EDGE_FOR), fv00);
//        my_hg->SetEdgeFeatures(HyperEdge(1,-1,1,HyperEdge::EDGE_FOR), fv11);
//        my_hg->SetEdgeFeatures(HyperEdge(2,-1,2,HyperEdge::EDGE_FOR), fv22);
//        my_hg->SetEdgeFeatures(HyperEdge(3,-1,3,HyperEdge::EDGE_FOR), fv33);
//        my_hg->SetEdgeFeatures(edge0_2b, fv0_2b);
//        my_hg->SetEdgeFeatures(edge1_3b, fv1_3b);
//        my_hg->SetEdgeFeatures(edge01, fv01);
//        my_hg->SetEdgeFeatures(edge23, fv23);
//        my_hg->SetEdgeFeatures(edge03sc, fv03sc);
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
        my_hg->SetStack(0,1, 3,3, ts03d); // only for saving feature
        my_hg->HyperGraph::SetStack(0,4,tsRoot);
        my_hg->SetSaveFeatures(true);
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
        // Make the hypergraph and get the features
        DiscontinuousHyperGraph hyper_graph(1);
        hyper_graph.SetSaveFeatures(true);
        // for Save{Striaght,Inverted}Features
        TargetSpan * span0_2 = new DiscontinuousTargetSpan(0,0, 2,2);
        hyper_graph.SetStack(0,0, 2,2, span0_2);
        // Generate the features
        const FeatureVectorInt * edge02int =
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge0_2);
        FeatureVectorString * edge02act =
                        mod.StringifyFeatureVector(*edge02int);
        // Do the parsing and checking
        int ret = 1;
        ret *= CheckVector(edge02exp, *edge02act);
        FeatureVectorInt edge02intexp;
        edge02intexp.push_back(MakePair(mod.GetFeatureIds().GetId("SW||he||rice||ID"), 1));
        edge02intexp.push_back(MakePair(mod.GetFeatureIds().GetId("SP||PRP||NN||ID"), 1));
        ret *= CheckVector(edge02intexp, *edge02int);
        // Generate the features again
        const FeatureVectorInt * edge02int2 =
                        hyper_graph.GetEdgeFeatures(mod, set, datas, edge0_2);
        // Make sure that the pointers are equal
        if(edge02int != edge02int2) {
            cerr << "Edge pointers are not equal." << endl;
            ret = 0;
        }
        mod.SetWeight("SW||he||rice||ID", 1);
        mod.SetWeight("SP||PRP||NN||ID", 2);
        // Check to make sure that the weights are Ok
        double weight_act = hyper_graph.GetEdgeScore(mod, set,
                                                     datas, edge0_2);
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
        stack0->AddHypothesis(new Hypothesis(1,1, 0,0, 0,0,HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(0, 0, stack0);
        stack1->AddHypothesis(new Hypothesis(2,2, 1,1, 1,1, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(1, 1, stack1);
        stack2->AddHypothesis(new Hypothesis(4,4, 2,2, 2,2, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(2, 2, stack2);
        stack3->AddHypothesis(new Hypothesis(8,8, 3,3, 3,3, HyperEdge::EDGE_FOR));
        graph.HyperGraph::SetStack(3, 3, stack3);
        // Try processing stack01, which lead to set stack0_2
        set.SetMaxTerm(0);
        TargetSpan *stack01 = new TargetSpan(0,1);
        graph.HyperGraph::SetStack(0, 1, stack01);
        TargetSpan *stack0_2 = new DiscontinuousTargetSpan(0,0, 2,2);
        graph.SetStack(0,0, 2,2, stack0_2);
        graph.ProcessOneSpan(model, set, datas, 0, 1);
        // The stack should contain two target spans (2,0) and (0,2),
        // each with one hypothesis
        int ret = 1;
        if(stack0_2->HypSize() != 2) {
            cerr << "stack0_2->size() != 2: " << stack0_2->HypSize() << endl; ret = 0;
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
        graph.SetBeamSize(4*3*2);
        graph.BuildHyperGraph(model, set, datas);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        TargetSpan * stack03 = graph.HyperGraph::GetStack(0, 3);
        TargetSpan * stackRoot = graph.HyperGraph::GetRoot();
        // The total number of stacks should be 11: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 root
        if(stacks.size() != 11) {
            cerr << "stacks.size() != 11: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 4*3*2: including non-ITGs
        } else if (stack03->HypSize() != 4*3*2) {
            cerr << "Root node stack03->size() != 4*3*2: " << stack03->HypSize()<< endl;
            BOOST_FOREACH(Hypothesis *hyp, stack03->GetHypotheses()){
            	DiscontinuousHypothesis * dhyp =
            							dynamic_cast<DiscontinuousHypothesis*>(hyp);
            	cerr << (dhyp ? *dhyp : *hyp);
            	hyp->PrintChildren(cerr);
            }
            ret = 0;
        } else if (stackRoot->HypSize() != stack03->HypSize()) {
            cerr << "Root hypotheses " << stackRoot->HypSize()
                 << " and root spans " << stack03->HypSize() << " don't match." << endl; ret = 0;
        }

        BOOST_FOREACH(TargetSpan * stack, stacks)
			BOOST_FOREACH(Hypothesis * hyp, stack->GetHypotheses()){
				Hypothesis * lhyp = hyp->GetLeftHyp();
				Hypothesis * rhyp = hyp->GetRightHyp();
				if (hyp->GetScore() !=
						hyp->GetSingleScore()
						+ (lhyp ? lhyp->GetScore() : 0)
						+ (rhyp ? rhyp->GetScore() : 0) ){
					cerr << "Incorrect viterbi score " << *hyp;
					hyp->PrintChildren(cerr);
					ret = 0;
				}
				if (hyp->GetTrgLeft() < hyp->GetLeft()
					|| hyp->GetRight() < hyp->GetTrgRight()){
					cerr << "Malformed hypothesis "<< *hyp;
					hyp->PrintChildren(cerr);
					ret = 0;
				}
				// root has no edge features
				if (hyp->GetEdgeType() == HyperEdge::EDGE_ROOT)
					continue;
				DiscontinuousHypothesis * dhyp =
						dynamic_cast<DiscontinuousHypothesis*>(hyp);
				const FeatureVectorInt *fvi = graph.GetEdgeFeatures(model, set, datas, *hyp->GetEdge());
				FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
				bool error = false;
				BOOST_FOREACH(FeaturePairString feat, *fvs){
					string edgetype = feat.first.substr(feat.first.size()-2);
					if (edgetype[0] != hyp->GetEdgeType()){
						cerr << "Incorrect edge type ";
						error = true;
					}
					if (edgetype[1] != hyp->GetEdge()->GetClass()){
						cerr << "Incorrect edge class ";
						error = true;
					}
				}
				if (error){
					cerr << (dhyp ? *dhyp : *hyp) << endl << *fvs << endl;
					ret = 0;
				}
				delete fvs;
			}
        return ret;
    }

    int TestBuildHyperGraphCubeGrowing() {
        DiscontinuousHyperGraph graph(1, true);
        set.SetMaxTerm(1);
        graph.SetBeamSize(4*3*2);
        graph.BuildHyperGraph(model, set, datas);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        TargetSpan * stack03 = graph.HyperGraph::GetStack(0, 3);
        TargetSpan * stackRoot = graph.HyperGraph::GetRoot();
        // The total number of stacks should be 11: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 root
        if(stacks.size() != 11) {
            cerr << "stacks.size() != 11: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 4*3*2: including non-ITGs
        } else if (stack03->HypSize() != 4*3*2) {
            cerr << "Root node stack03->size() != 4*3*2: " << stack03->HypSize()<< endl;
            BOOST_FOREACH(const Hypothesis *hyp, stack03->GetHypotheses())
            	cerr << " " << hyp->GetTrgLeft() << "-" <<hyp->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->HypSize() != stack03->HypSize()) {
            cerr << "Root hypotheses " << stackRoot->HypSize()
                 << " and root spans " << stack03->HypSize() << " don't match." << endl; ret = 0;
        }

        for(int i = 0; i < 4*3*2; i++){
        	int pop_count = 0;
        	Hypothesis * hyp = graph.LazyKthBest(stackRoot, i, model, datas, pop_count);
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
			if (hyp->GetTrgLeft() < hyp->GetLeft()
				|| hyp->GetRight() < hyp->GetTrgRight()){
				cerr << "Malformed hypothesis "<< *hyp;
				hyp->PrintChildren(cerr);
				ret = 0;
			}
        	// root has no edge features
        	if (hyp->GetEdgeType() == HyperEdge::EDGE_ROOT)
        		continue;
        	DiscontinuousHypothesis * dhyp =
        			dynamic_cast<DiscontinuousHypothesis*>(hyp);
        	const FeatureVectorInt *fvi = graph.GetEdgeFeatures(model, set, datas, *hyp->GetEdge());
        	FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
        	bool error = false;
        	BOOST_FOREACH(FeaturePairString feat, *fvs){
        		string edgetype = feat.first.substr(feat.first.size()-2);
        		if (edgetype[0] != hyp->GetEdgeType()){
        			cerr << "Incorrect edge type ";
        			error = true;
        		}
        		if (edgetype[1] != hyp->GetEdge()->GetClass()){
        			cerr << "Incorrect edge class ";
        			error = true;
        		}
        	}
        	if (error){
        		cerr << (dhyp ? *dhyp : *hyp) << endl << *fvs << endl;
        		ret = 0;
        	}
        	delete fvs;
        }
        return ret;
    }

    int TestBuildHyperGraphMultiThreads() {
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
        featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
        FeatureDataSequence sent;
        sent.FromString("t h i s i s a v e r y l o n g s e n t e n c e .");
        vector<FeatureDataBase*> datas;
        datas.push_back(&sent);
    	struct timespec tstart={0,0}, tend={0,0};
        ReordererModel model;

    	DiscontinuousHyperGraph graph(2, true, true);
        int beam_size = 100;
        graph.SetBeamSize(beam_size);
    	graph.SetThreads(1);
//    	graph.SetVerbose(2);

        clock_gettime(CLOCK_MONOTONIC, &tstart);
        graph.BuildHyperGraph(model, set, datas);
		clock_gettime(CLOCK_MONOTONIC, &tend);
		double thread1 = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
				((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);

		// reusing model requires no more model->GetFeatureIds()->GetId(add=true)
		// therefore, it will be much faster
		graph.SetThreads(4);
		graph.ClearStacks();
    	clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(model, set, datas);
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
    	} else if (graph.GetRoot()->HypSize() != beam_size) {
    		cerr << "root stacks->size() != " << beam_size << ": " <<graph.GetRoot()->HypSize()<< endl;
    		BOOST_FOREACH(const Hypothesis *hyp, graph.GetRoot()->GetHypotheses()){
    			cerr << *hyp <<  endl;
    		}
    		ret = 0;
    	}
		if(thread4 > thread1){
			cerr << "more threads, more time? " << thread4 << " > " << thread1 << endl;
			ret = 0;
		}
		for(int i = 0; i < beam_size; i++){
			int pop_count = 0;
			Hypothesis * hyp = graph.LazyKthBest(graph.GetRoot(), i, model, datas, pop_count);
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
			if (hyp->GetTrgLeft() < hyp->GetLeft()
			|| hyp->GetRight() < hyp->GetTrgRight()){
				cerr << "Malformed hypothesis "<< *hyp;
				hyp->PrintChildren(cerr);
				ret = 0;
			}
			// root has no edge features
			if (hyp->GetEdgeType() == HyperEdge::EDGE_ROOT)
				continue;
			DiscontinuousHypothesis * dhyp =
					dynamic_cast<DiscontinuousHypothesis*>(hyp);
			const FeatureVectorInt *fvi = graph.GetEdgeFeatures(model, set, datas, *hyp->GetEdge());
			FeatureVectorString *fvs = model.StringifyFeatureVector(*fvi);
			bool error = false;
			BOOST_FOREACH(FeaturePairString feat, *fvs){
				string edgetype = feat.first.substr(feat.first.size()-2);
				if (edgetype[0] != hyp->GetEdgeType()){
					cerr << "Incorrect edge type ";
					error = true;
				}
				if (edgetype[1] != hyp->GetEdge()->GetClass()){
					cerr << "Incorrect edge class ";
					error = true;
				}
			}
			if (error){
				cerr << (dhyp ? *dhyp : *hyp) << endl << *fvs << endl;
				ret = 0;
			}
			delete fvs;
		}
    	set.SetUseReverse(true);
    	return ret;
    }

    int TestBuildHyperGraphSaveFeatures() {
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
    	featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
    	FeatureDataSequence sent;
    	sent.FromString("t h i s i s a v e r y l o n g s e n t e n c e .");
    	vector<FeatureDataBase*> datas;
    	datas.push_back(&sent);
    	struct timespec tstart={0,0}, tend={0,0};

    	DiscontinuousHyperGraph graph(3, 1, true, true);
    	int beam_size = 100;
    	graph.SetSaveFeatures(true);
        graph.SetBeamSize(beam_size);
        graph.SetNumWords(sent.GetNumWords());
        graph.SetAllStacks();
    	ReordererModel model;

    	clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(model, set, datas);
    	clock_gettime(CLOCK_MONOTONIC, &tend);
    	double before_save = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
    			((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);

    	// reusing saved features does not access to model->GetFeatureIds() anymore
    	// therefore, it will be much faster
    	ReordererModel empty_model; // use empty model
    	empty_model.SetAdd(false); // do not allow adding feature ids anymore
    	clock_gettime(CLOCK_MONOTONIC, &tstart);
    	graph.BuildHyperGraph(empty_model, set, datas);
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
    	} else if (graph.GetRoot()->HypSize() != beam_size) {
    		cerr << "root stacks->size() != " << beam_size << ": " <<graph.GetRoot()->HypSize()<< endl;
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


    int TestBuildHyperGraphSerialize() {
    	DiscontinuousHyperGraph graph1(3, true, true);
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
    	featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
    	FeatureDataSequence sent;
    	sent.FromString("t h i s i s a v e r y l o n g s e n t e n c e .");
    	vector<FeatureDataBase*> datas;
    	datas.push_back(&sent);
    	int beam_size = 100;
    	graph1.SetBeamSize(beam_size);
    	ReordererModel model;
    	graph1.SetSaveFeatures(true);
    	graph1.SetNumWords(sent.GetNumWords());
    	graph1.SetAllStacks();
    	graph1.SetThreads(4);
    	graph1.BuildHyperGraph(model, set, datas);

    	ofstream out("/tmp/feature.discontinuous");
    	graph1.FeaturesToStream(out);
    	out.close();

    	DiscontinuousHyperGraph graph2(3, 1, true, true);
    	graph2.SetBeamSize(beam_size);
    	graph2.SetSaveFeatures(true); // use saved feature
    	ReordererModel empty_model; // use empty model
    	empty_model.SetAdd(false); // do not allow adding feature ids anymore
    	int N = sent.GetNumWords();
    	graph2.SetNumWords(N);
    	graph2.SetAllStacks();
    	ifstream in("/tmp/feature.discontinuous");
    	graph2.FeaturesFromStream(in);
    	in.close();
    	graph2.SetThreads(4);
    	graph2.BuildHyperGraph(empty_model, set, datas);
    	const std::vector<TargetSpan*> & stacks = graph2.GetStacks();
    	int ret = 1;

    	FeatureVectorInt * fvi1, *fvi2;
    	int D = graph2.gap_size_;
    	for(int L = 1; L <= N; L++) {
			for(int l = 0; l <= N-L; l++){
				int r = l+L-1;
//    			cout << "Check " << *graph1.HyperGraph::GetStack(l, r) << endl;
				ret = CheckStackEqual(graph1.HyperGraph::GetStack(l, r), graph2.HyperGraph::GetStack(l, r));
				for(int m = l + 1;m <= r;m++){
					for (int c = m+1; c-m <= D; c++)
						for(int n = c+1; n-c <= D && n <= r;n++){
//    						cout << "Check " << *(DiscontinuousTargetSpan*)graph1.GetStack(l, m, n, r) << endl;
							ret = CheckStackEqual(graph1.GetStack(l, m, n, r),
									graph2.GetStack(l, m, n, r));
						}
					for(int d = 1;d <= D;d++){
						if(IsXXD(l, m - 1, m + d, r + d, D, N)){
//    						cout << "Check " << *(DiscontinuousTargetSpan*)graph1.GetStack(l, m - 1, m + d, r + d) << endl;
							ret = CheckStackEqual(graph1.GetStack(l, m - 1, m + d, r + d),
									graph2.GetStack(l, m - 1, m + d, r + d));
						}
					}
				}
			}
		}
    	set.SetUseReverse(true);
    	return ret;
    }

    int TestBuildHyperGraphSaveAllFeatures() {
    	DiscontinuousHyperGraph graph1(3, true, true);
    	FeatureSet set;
    	FeatureSequence * featw = new FeatureSequence;
    	featw->ParseConfiguration("SW%LS%RS");
    	set.AddFeatureGenerator(featw);
    	set.SetMaxTerm(0);
    	set.SetUseReverse(false);
    	FeatureDataSequence sent;
    	sent.FromString("t h i s i s a v e r y l o n g s e n t e n c e .");
    	vector<FeatureDataBase*> datas;
    	datas.push_back(&sent);
    	int beam_size = 100;
    	graph1.SetBeamSize(beam_size);
    	ReordererModel model;
    	graph1.SetSaveFeatures(true);
    	graph1.SetNumWords(sent.GetNumWords());
    	graph1.SetAllStacks();
    	graph1.SaveAllEdgeFeatures(model, set, datas);

    	ofstream out("/tmp/feature.discontinuous.saveall");
    	graph1.FeaturesToStream(out);
    	out.close();

    	DiscontinuousHyperGraph graph2(3, 1, true, true);
    	graph2.SetBeamSize(beam_size);
    	graph2.SetSaveFeatures(true); // use saved feature
    	ReordererModel empty_model; // use empty model
    	empty_model.SetAdd(false); // do not allow adding feature ids anymore
    	int N = sent.GetNumWords();
    	graph2.SetNumWords(N);
    	graph2.SetAllStacks();
    	ifstream in("/tmp/feature.discontinuous.saveall");
    	graph2.FeaturesFromStream(in);
    	in.close();
    	graph2.SetThreads(4);
    	graph2.BuildHyperGraph(empty_model, set, datas);
    	const std::vector<TargetSpan*> & stacks = graph2.GetStacks();
    	int ret = 1;

    	FeatureVectorInt * fvi1, *fvi2;
    	int D = graph2.gap_size_;
    	for(int L = 1; L <= N; L++) {
    		for(int l = 0; l <= N-L; l++){
    			int r = l+L-1;
//    			cout << "Check " << *graph1.HyperGraph::GetStack(l, r) << endl;
    			ret = CheckStackEqual(graph1.HyperGraph::GetStack(l, r), graph2.HyperGraph::GetStack(l, r));
    			for(int m = l + 1;m <= r;m++){
    				for (int c = m+1; c-m <= D; c++)
    					for(int n = c+1; n-c <= D && n <= r;n++){
//    						cout << "Check " << *(DiscontinuousTargetSpan*)graph1.GetStack(l, m, n, r) << endl;
    						ret = CheckStackEqual(graph1.GetStack(l, m, n, r),
    								graph2.GetStack(l, m, n, r));
    					}
    				for(int d = 1;d <= D;d++){
    					if(IsXXD(l, m - 1, m + d, r + d, D, N)){
//    						cout << "Check " << *(DiscontinuousTargetSpan*)graph1.GetStack(l, m - 1, m + d, r + d) << endl;
    						ret = CheckStackEqual(graph1.GetStack(l, m - 1, m + d, r + d),
    								graph2.GetStack(l, m - 1, m + d, r + d));
    					}
    				}
    			}
    		}
    	}
    	set.SetUseReverse(true);
    	return ret;
    }
    int TestBuildHyperGraphGap2() {
    	// use full-fledged version
        DiscontinuousHyperGraph graph(2, true, true);
        set.SetMaxTerm(1);
        ReordererModel model;
        FeatureDataSequence sent, sent_pos;
        sent.FromString("this sentence has five words");
		sent_pos.FromString("PRP NNS VB ADJ NNP");
        vector<FeatureDataBase*> datas;
        datas.push_back(&sent);
        datas.push_back(&sent_pos);
        graph.SetBeamSize(5*4*3*2);
        graph.BuildHyperGraph(model, set, datas);
        const std::vector<TargetSpan*> & stacks = graph.GetStacks();
        int ret = 1;
        TargetSpan * stack04 = graph.HyperGraph::GetStack(0, 4);
        TargetSpan * stackRoot = graph.HyperGraph::GetStack(-1, 4);
        // The total number of stacks should be 6*5/2 + 1: 0-0 0-1 1-1 0-2 1-2 2-2 0-3 1-3 2-3 3-3 0-4 1-4 2-4 3-4 4-4 root
        if(stacks.size() != 16) {
            cerr << "stacks.size() != 16: " << stacks.size() << endl; ret = 0;
        // The number of target spans should be 5*4*3*2 =720:
        } else if (stack04->HypSize() != 5*4*3*2) {
            cerr << "Root node stack04->size() != 720: " << stack04->HypSize()<< endl;
            BOOST_FOREACH(const Hypothesis *hyp, stack04->GetHypotheses())
            	cerr << " " << hyp->GetTrgLeft() << "-" <<hyp->GetTrgRight() << endl;
            ret = 0;
        } else if (stackRoot->HypSize() != stack04->HypSize()) {
            cerr << "Root hypotheses " << stackRoot->HypSize()
                 << " and root spans " << stack04->HypSize() << " don't match." << endl; ret = 0;
        }
        return ret;
    }

    int TestBuildHyperGraphSize1() {
		DiscontinuousHyperGraph graph(0);
		set.SetMaxTerm(0);
		ReordererModel model;
		FeatureDataSequence sent, sent_pos;
		sent.FromString("one");
		sent_pos.FromString("NNS");
		vector<FeatureDataBase*> datas;
		datas.push_back(&sent);
		datas.push_back(&sent_pos);
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
    	FeatureVectorInt act;
    	std::tr1::unordered_map<int, double> feat_map;
    	my_hg->AccumulateFeatures(feat_map, model, set, datas, my_hg->GetBest());
    	ClearAndSet(act, feat_map);
        FeatureVectorInt exp;
        exp.push_back(MakePair(1,1));
        exp.push_back(MakePair(2,1));
        exp.push_back(MakePair(3,1));
        exp.push_back(MakePair(4,1));
        exp.push_back(MakePair(5,1));
        exp.push_back(MakePair(6,1));
        exp.push_back(MakePair(8,1));
        exp.push_back(MakePair(10,1*7));
        // Test the rescoring
        return CheckVector(exp, act);
    }

    int TestAddLoss() {
    	DiscontinuousHyperGraph graph(1);
    	set.SetMaxTerm(1);
        graph.SetBeamSize(4*3*2);
    	graph.BuildHyperGraph(model, set, datas);
    	LossChunk loss;
    	loss.Initialize(&ranks, NULL);
    	graph.AddLoss(&loss, &ranks, NULL);
    	TargetSpan * span1_3 = graph.GetStack(1,1, 3,3);
    	int ret = 1;
    	BOOST_FOREACH(Hypothesis * hyp, span1_3->GetHypotheses()){
    		if (hyp->GetEdgeType() == HyperEdge::EDGE_STR && hyp->GetLoss() != 1){
    			cerr << "Expect loss 1 != " << hyp->GetLoss() << *hyp << endl;
    			ret = 0;
    		}
    		if (hyp->GetEdgeType() == HyperEdge::EDGE_INV && hyp->GetLoss() != 0){
    			cerr << "Expect loss 0 != " << hyp->GetLoss() << *hyp << endl;
    			ret = 0;
    		}
    	}

    	return ret;
    }
    // Test that rescoring works
    int TestRescore() {
        // Create a model that assigns a weight of -1 to each production
//        ReordererModel mod;
//        for(int i = 0; i < 20; i++) {
//            ostringstream oss;
//            oss << "WEIGHT" << i;
//            mod.SetWeight(oss.str(), 0);
//        }
//        mod.SetWeight("WEIGHT10", -1);
        int ret = 1;
        // Simply rescoring with this model should pick the forward production
        // with a score of -3
        double score = my_hg->Rescore(0.0);
        if(score != -3) {
            cerr << "Rescore(mod, 0.0) != -3: " << score << endl; ret = 0;
        }
        // Rescoring with loss +1 should pick the non-ITG
        // with a loss of 1+2+3+4+5+6+7, minus a weight of 1*7 -> 21
        score = my_hg->Rescore(1.0);
        if(score != 21) {
            cerr << "Rescore(mod, 1.0) != 21: " << score << endl; ret = 0;
        }
        // Rescoring with loss -1 should pick the ITG
		// with a loss of -1-2-7, minus a weight of 1*7 -> 21
		score = my_hg->Rescore(-1);
		if(score != -17) {
			cerr << "Rescore(mod, -1) != -17: " << score << endl; ret = 0;
		}
        return ret;
    }

    // Test various types of reordering in the hypergraph
    int TestReorderingAndPrint() {
        // Create the expected reordering Inside-Out vectors
        vector<int> vec1302(4,0); vec1302[0] = 1; vec1302[1] = 3; vec1302[2] = 0; vec1302[3] = 2;
        vector<int> vec2031(4,0); vec2031[0] = 2; vec2031[1] = 0; vec2031[2] = 3; vec2031[3] = 1;
        vector<string> str(4); str[0] = "0"; str[1] = "1"; str[2] = "2"; str[3] = "3";
        // Create a forest that can handle various things
        TargetSpan *span00 = new TargetSpan(0,0),
                   *span11 = new TargetSpan(1,1),
                   *span22 = new TargetSpan(2,2),
                   *span33 = new TargetSpan(3,3),
                   *span0_2 = new DiscontinuousTargetSpan(0,0, 2,2),
                   *span1_3 = new DiscontinuousTargetSpan(1,1, 3,3),
                   *span03 = new TargetSpan(0,3),
                   *spanr = new TargetSpan(0,3);
        span00->AddHypothesis(new Hypothesis(1,1, 0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(new Hypothesis(1,1, 1,1,-1,-1,HyperEdge::EDGE_FOR));
        span22->AddHypothesis(new Hypothesis(1,1, 2,2,-1,-1,HyperEdge::EDGE_FOR));
        span33->AddHypothesis(new Hypothesis(1,1, 3,3,-1,-1,HyperEdge::EDGE_FOR));
        span0_2->AddHypothesis(new DiscontinuousHypothesis(1,1,
        		0,0, 2,2, 0,2, HyperEdge::EDGE_STR, -1,
        		0,0, span00, span22));
        span0_2->AddHypothesis(new DiscontinuousHypothesis(1,1,
        		0,0, 2,2, 2,0, HyperEdge::EDGE_INV, -1,
        		0,0, span00, span22));
        span1_3->AddHypothesis(new DiscontinuousHypothesis(1,1,
        		1,1, 3,3, 1,3, HyperEdge::EDGE_STR, -1,
        		0,0, span11, span33));
        span1_3->AddHypothesis(new DiscontinuousHypothesis(1,1,
        		1,1, 3,3, 3,1, HyperEdge::EDGE_INV, -1,
        		0,0, span11, span33));
        span03->AddHypothesis(new Hypothesis(1,1, 0,3, 1,2, HyperEdge::EDGE_INV,-1, 0,0, span0_2, span1_3));
        span03->AddHypothesis(new Hypothesis(1,1, 0,3, 2,1, HyperEdge::EDGE_STR,-1, 1,1, span0_2, span1_3));
        spanr->AddHypothesis(new Hypothesis(1,1, 0,3, 1,2, HyperEdge::EDGE_ROOT,-1,0,-1, span03));
        spanr->AddHypothesis(new Hypothesis(1,1, 0,3, 2,1, HyperEdge::EDGE_ROOT,-1,1,-1, span03));
        DiscontinuousHyperGraph graph(1);
        graph.HyperGraph::SetStack(0,0, span00);
        graph.HyperGraph::SetStack(1,1, span11);
        graph.HyperGraph::SetStack(2,2, span22);
        graph.HyperGraph::SetStack(3,3, span33);
        graph.SetStack(0,0, 2,2, span0_2);
        graph.SetStack(1,1, 3,3, span1_3);
        // Get the reordering for 1302
        int ret = 1;
        vector<int> reorder1302;
        spanr->GetHypothesis(0)->GetReordering(reorder1302);
        ostringstream for_oss; spanr->GetHypothesis(0)->PrintParse(str, for_oss);
        ret = min(ret, CheckVector(vec1302, reorder1302));
        ret = min(ret, CheckString("(I (S (F (FW 0)) (F (FW 2))) (S (F (FW 1)) (F (FW 3))))", for_oss.str()));
        // Get the reordering for 2031
        vector<int> reorder2031;
        spanr->GetHypothesis(1)->GetReordering(reorder2031);
        ostringstream bac_oss; spanr->GetHypothesis(1)->PrintParse(str, bac_oss);
        ret = min(ret, CheckVector(vec2031, reorder2031));
        ret = min(ret, CheckString("(S (I (F (FW 0)) (F (FW 2))) (I (F (FW 1)) (F (FW 3))))", bac_oss.str()));
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
        span00->AddHypothesis(new Hypothesis(1,1, 0,0,-1,-1,HyperEdge::EDGE_FOR));
        span11->AddHypothesis(new Hypothesis(1,1, 1,1,-1,-1,HyperEdge::EDGE_BAC));
        span01->AddHypothesis(new Hypothesis(1,1, 0,1,-1,-1,HyperEdge::EDGE_INV,-1,1,-1,span00,span11));
        spanr->AddHypothesis(new Hypothesis(1,1, 0,1,-1,-1,HyperEdge::EDGE_ROOT,-1,0,-1,span01));
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
        span01->AddHypothesis(new Hypothesis(1,1, 0,1,-1,-1,HyperEdge::EDGE_STR,-1,1,-1,span00,span11));
        // Print again
        ostringstream graph_stream;
        hg.PrintHyperGraph(str01, graph_stream);
        ret = min(ret, CheckString("{\"rules\": [\"[F] ||| 0\", \"[B] ||| 1\", \"[I] ||| [F] [B]\", \"[S] ||| [F] [B]\", \"[R] ||| [I]\", \"[R] ||| [S]\"], \"nodes\": [[{\"feature\":{\"parser\":1},\"rule\":1}], [{\"feature\":{\"parser\":1},\"rule\":2}], [{\"tail\":[0,1],\"feature\":{\"parser\":1},\"rule\":3}], [{\"tail\":[0,1],\"feature\":{\"parser\":1},\"rule\":4}], [{\"tail\":[2],\"feature\":{\"parser\":1},\"rule\":5}, {\"tail\":[3],\"feature\":{\"parser\":1},\"rule\":6}]], \"goal\": 4}",graph_stream.str()));
        return ret;
    }

    bool RunTest() {
        int done = 0, succeeded = 0;
        done++; cout << "TestGetTrgSpanID()"; if(TestGetTrgSpanID()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestGetEdgeFeaturesAndWeights()"; if(TestGetEdgeFeaturesAndWeights()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestProcessOneSpan()"; if(TestProcessOneSpan()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraph()"; if(TestBuildHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphCubeGrowing()"; if(TestBuildHyperGraphCubeGrowing()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphMultiThreads()"; if(TestBuildHyperGraphMultiThreads()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphSaveFeatures()" << endl; if(TestBuildHyperGraphSaveFeatures()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphSerialize()" << endl; if(TestBuildHyperGraphSerialize()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestBuildHyperGraphSaveAllFeatures()" << endl; if(TestBuildHyperGraphSaveAllFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
        done++; cout << "TestAddLoss()"; if(TestAddLoss()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestAccumulateLoss()"; if(TestAccumulateLoss()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestAccumulateFeatures()"; if(TestAccumulateFeatures()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestRescore()"; if(TestRescore()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestReorderingAndPrint()"; if(TestReorderingAndPrint()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestPrintHyperGraph()"; if(TestPrintHyperGraph()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphGap2()"; if(TestBuildHyperGraphGap2()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        done++; cout << "TestBuildHyperGraphSize1()"; if(TestBuildHyperGraphSize1()) succeeded++; else cout << "FAILED!!!" << endl; cout << "Done" << endl;
        cout << "#### TestDiscontinuousHyperGraph Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
        return done == succeeded;
    }

private:
    HyperEdge edge00, edge11, edge22, edge33, edge01, edge23, edge03sc;
    DiscontinuousHyperEdge edge0_2, edge1_3, edge03sd;
    CombinedAlign cal;
    Ranks ranks;
    FeatureDataSequence sent, sent_pos;
    ReordererModel model;
    std::vector<double> weights;
    FeatureSet set;
    vector<FeatureDataBase*> datas;
    FeatureSequence *featw, *featp;
    TargetSpan *ts00, *ts11, *ts22, *ts33, *ts03, *ts01, *ts23, *tsRoot;
    DiscontinuousTargetSpan *ts1_3, *ts0_2, *ts03d;
    DiscontinuousHyperGraph *my_hg;


};

}

#endif
