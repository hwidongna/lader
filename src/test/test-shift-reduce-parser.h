/*
 * test-shift-reduce-parser.h
 *
 *  Created on: Dec 27, 2013
 *      Author: leona
 */

#ifndef TEST_SHIFT_REDUCE_PARSER_H_
#define TEST_SHIFT_REDUCE_PARSER_H_

#include "test-base.h"
#include <lader/combined-alignment.h>
#include <lader/feature-sequence.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-set.h>
#include <fstream>

namespace lader {

class TestShiftReduceParser : public TestBase {

public:

    TestShiftReduceParser(){
    	// Create a combined alignment
		//  x...
		//  ..x.
		//  .x..
    	//  ...x
		vector<string> words(4, "x");
		Alignment al(MakePair(4,4));
		al.AddAlignment(MakePair(0,0));
		al.AddAlignment(MakePair(1,2));
		al.AddAlignment(MakePair(2,1));
		al.AddAlignment(MakePair(3,3));
		cal = Ranks(CombinedAlign(words,al));
		// Create a sentence
		string str = "he ate rice .";
		sent.FromString(str);
        string str_pos = "PRP VBD NN .";
        sent_pos.FromString(str_pos);
    }
    ~TestShiftReduceParser() { }

    int TestGetReordering() {
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	vector<DPState::Action> refseq = cal.GetReference();
    	stateseq.push_back(new DPState());
    	for (int step = 1 ; step < 2*n ; step++){
    		DPState * state = stateseq.back();
    		state->Take(refseq[step-1], stateseq, true);
    	}
    	DPState * goal = stateseq.back();
    	vector<int> act;
    	goal->GetReordering(act);
    	vector<int> exp(n);
    	exp[0]=0, exp[1]=2, exp[2]=1, exp[3]=3;
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	int ret = 1;
    	ret *= CheckVector(exp, act);
    	return ret;
    }

    int TestSetSignature() {
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	vector<DPState::Action> refseq = cal.GetReference();
    	stateseq.push_back(new DPState());
    	DPState * state = stateseq.back();
    	int max = 3;
    	for (int step = 1 ; step < 2*n ; step++){
    		state->Take(refseq[step-1], stateseq, true);
    		state = stateseq.back();
    		state->SetSignature(max);
    	}
    	vector<Span> act = state->GetSignature(); // a reduce state for the whole sentence
		vector<Span> exp(1); // only one elements in signature
		exp[0]=MakePair(0,3);
		int ret = 1;
		ret *= CheckVector(exp, act);
		if (!ret)
			cerr << "SetSignature fails: " << *state << endl;

    	state = stateseq[2*n-2]; // a shifted state after reduce
    	act = state->GetSignature();
    	exp.resize(2); // only two elements in signature
    	exp[0]=MakePair(3,3), exp[1]=MakePair(0,1); // from stack top
    	ret *= CheckVector(exp, act);
    	if (!ret)
    		cerr << "SetSignature fails: " << *state << endl;

    	state = stateseq[3]; // a 3*shifted state
    	act = state->GetSignature();
    	exp.resize(3);
    	exp[0]=MakePair(2,2), exp[1]=MakePair(1,1), exp[2]=MakePair(0,0);
    	ret *= CheckVector(exp, act);
    	if (!ret)
    		cerr << "SetSignature fails: " << *state << endl;

    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return ret;
    }

    int TestAllActions() {
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	vector<DPState::Action> refseq = cal.GetReference();
    	vector<DPState::Action> exp(2*n-1, DPState::SHIFT);
    	exp[3]=DPState::INVERTED, exp[4]=DPState::STRAIGTH, exp[6]=DPState::STRAIGTH;    	
    	int ret = 1;
    	ret *= CheckVector(exp, refseq);
    	if (!ret){
    		cerr << "incorrect reference sequence" << endl;
    		return 0;
    	}
    	
    	stateseq.push_back(new DPState());
    	for (int step = 1 ; step < 2*n ; step++){
    		DPState * state = stateseq.back();
    		state->Take(refseq[step-1], stateseq, true);
    	}
    	// for a complete tree
    	DPState * goal = stateseq.back();
    	if (!goal->IsGold()){
    		cerr << *goal << endl;
    		return 0;
    	}
    	vector<DPState::Action> act;
    	goal->AllActions(act);
    	if (act.size() != 2*n-1){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		return 0;
    	}
    	ret *= CheckVector(refseq, act);
    	if (!ret){
    		cerr << "incorrect all actions" << endl;
    		return 0;
    	}
    	// for an incomplete tree
    	act.clear();
    	goal = stateseq[2*n - 2];
    	goal->AllActions(act);
    	if (act.size() != 2*n-2){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		return 0;
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return ret;
    }

    int TestShiftM() {
    	// Create a combined alignment
		//  ..x.
		//  ...x
		//  x...
    	//  .x..
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,4));
    	al.AddAlignment(MakePair(0,2));
    	al.AddAlignment(MakePair(1,3));
    	al.AddAlignment(MakePair(2,0));
    	al.AddAlignment(MakePair(3,1));
    	Ranks cal;
    	FeatureDataSequence sent, sent_pos;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "this block is swapped";
    	sent.FromString(str);
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	vector<DPState::Action> refseq = cal.GetReference();
		vector<DPState::Action> exp(2*n-1, DPState::SHIFT);
		exp[2]=DPState::STRAIGTH, exp[5]=DPState::STRAIGTH, exp[6]=DPState::INVERTED;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

    	stateseq.push_back(new DPState());
    	stateseq.back()->Take(DPState::SHIFT, stateseq, true, 2); // shift-2
    	stateseq.back()->Take(DPState::SHIFT, stateseq, true, 2); // shift-2
    	stateseq.back()->Take(DPState::INVERTED, stateseq, true);
    	// for a complete tree
    	DPState * goal = stateseq.back();
    	if (!goal->IsGold()){
    		cerr << *goal << endl;
    		return 0;
    	}
    	vector<DPState::Action> act;
    	goal->AllActions(act);
    	if (act.size() != 2*n-1){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		return 0;
    	}
    	ret *= CheckVector(refseq, act);
    	if (!ret){
    		cerr << "incorrect all actions" << endl;
    		return 0;
    	}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=2, exp[1]=3, exp[2]=0, exp[3]=1;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				return 0;
			}
    	}

    	// for an incomplete tree
    	act.clear();
    	goal = stateseq[2];
    	goal->AllActions(act);
    	if (act.size() != 2*n-2){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		return 0;
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestGetReordering()" << endl; if(TestGetReordering()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSetSignature()" << endl; if(TestSetSignature()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestAllActions()" << endl; if(TestAllActions()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestShfitM()" << endl; if(TestShiftM()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestShiftReduceParser Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent, sent_pos;
};
}
#endif /* TEST_SHIFT_REDUCE_PARSER_H_ */