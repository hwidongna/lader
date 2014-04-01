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
#include <shift-reduce-dp/dparser.h>
#include <shift-reduce-dp/iparser.h>
#include <shift-reduce-dp/iparser-ranks.h>
#include <fstream>
#include <vector>

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
    }
    ~TestShiftReduceParser() { }

    int TestGetReordering() {
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
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
    	ActionVector refseq = cal.GetReference();
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
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
    	ActionVector exp(2*n-1, DPState::SHIFT);
    	exp[3]=DPState::INVERTED, exp[4]=DPState::STRAIGTH, exp[6]=DPState::STRAIGTH;    	
    	int ret = 1;
    	ret *= CheckVector(exp, refseq);
    	if (!ret){
    		cerr << "incorrect reference sequence" << endl;
    		return 0;
    	}
    	
    	Parser p;
    	DPState * goal = p.GuidedSearch(refseq, n);

    	if (!goal->IsGold()){
    		cerr << *goal << endl;
    		return 0;
    	}
    	ActionVector act;
    	goal->AllActions(act);
    	if (act.size() != 2*n-1){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
    	if (!ret){
    		cerr << "incorrect all actions" << endl;
    		ret = 0;
    	}
    	// for an incomplete tree
    	act.clear();
    	goal = p.GetBeamBest(2*n-2);
    	goal->AllActions(act);
    	if (act.size() != 2*n-2){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		ret = 0;
    	}
    	return ret;
    }

    int TestSearch() {
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
    	ShiftReduceModel model;
    	model.SetMaxState(3);
    	FeatureSet set;
        set.AddFeatureGenerator(new FeatureSequence);
        Sentence datas;
        datas.push_back(&sent);
    	Parser p;
    	p.Search(model, set, datas);
    	vector<Parser::Result> kbest;
    	p.GetKbestResult(kbest);
    	int ret = 1;
    	if (kbest.size() != n*(n-1)){ // 0-1 1-0 0-2 2-0 0-3 3-0 1-2 2-1 1-3 3-1 2-3 3-2
    		cerr << "kbest size " << kbest.size() << " != " << n*(n-1) << endl;
    		ret = 0;
    	}
    	for (int k = 0 ; k < kbest.size() ; k++){
    		DPState * goal = p.GetKthBest(k);
    		if (goal->GetStep() != refseq.size()){
    			cerr << "Goal step " << *goal << " != " << refseq.size() << endl;
    			ret = 0;
    		}
    		ActionVector act;
    		goal->AllActions(act);
    		if (act.size() != refseq.size()){
    			cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    			ret = 0;
    		}
    	}
    	return ret;
    }

    int TestSearch2() {
    	// Create a combined alignment
		//  x......
		//  ....xx.
		//  .......
    	//  ..x....
    	//  ......x
    	vector<string> words(5, "x");
    	Alignment al(MakePair(5,7));
    	al.AddAlignment(MakePair(0,0));
    	al.AddAlignment(MakePair(1,4));
    	al.AddAlignment(MakePair(1,5));
    	al.AddAlignment(MakePair(3,2));
    	al.AddAlignment(MakePair(4,6));
    	Ranks cal;
    	FeatureDataSequence sent;
    	cal = Ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT));
    	// Create a sentence
    	string str = "she threw the ball .";
    	sent.FromString(str);
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
		ActionVector exp(2*n-1, DPState::SHIFT);
		exp[4]=DPState::STRAIGTH; exp[5]=DPState::INVERTED;
		exp[6]=DPState::STRAIGTH; exp[8]=DPState::STRAIGTH;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

    	ShiftReduceModel model;
    	model.SetMaxState(3);
    	FeatureSet set;
    	set.AddFeatureGenerator(new FeatureSequence);
    	Sentence datas;
    	datas.push_back(&sent);
    	Parser p;
    	p.Search(model, set, datas);
    	vector<Parser::Result> kbest;
    	p.GetKbestResult(kbest);
    	if (kbest.size() != n*(n-1)){ // 0-1 1-0 0-2 2-0 0-3 3-0 1-2 2-1 1-3 3-1 2-3 3-2
    		cerr << "kbest size " << kbest.size() << " != " << n*(n-1) << endl;
    		ret = 0;
    	}

		for (int k = 0 ; k < kbest.size() ; k++){
			DPState * goal = p.GetKthBest(k);
			if (goal->GetStep() != refseq.size()){
				cerr << "Goal step " << *goal << " != " << refseq.size() << endl;
				ret = 0;
			}
			ActionVector act;
			goal->AllActions(act);
			if (act.size() != refseq.size()){
				cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
				ret = 0;
			}
		}
    	return ret;
    }

    int TestShift2() {
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
    	FeatureDataSequence sent;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "this block is swapped";
    	sent.FromString(str);
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
		ActionVector exp(2*n-1, DPState::SHIFT);
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
    		ret = 0;
    	}
    	ActionVector act;
    	goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
    	if (!ret){
    		cerr << "incorrect all actions" << endl;
    		ret = 0;
    	}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=2, exp[1]=3, exp[2]=0, exp[3]=1;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}

    	// for an incomplete tree
    	act.clear();
    	goal = stateseq[2];
    	goal->AllActions(act);
    	if (act.size() != 2*n-2){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		ret = 0;
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return ret;
    }

    int TestShift3() {
    	// Create a combined alignment
		//  .x..
		//  ..x.
		//  ...x
    	//  x...
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,4));
    	al.AddAlignment(MakePair(0,1));
    	al.AddAlignment(MakePair(1,2));
    	al.AddAlignment(MakePair(2,3));
    	al.AddAlignment(MakePair(3,0));
    	Ranks cal;
    	FeatureDataSequence sent;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "let's shift 3 words";
    	sent.FromString(str);
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
		ActionVector exp(2*n-1, DPState::SHIFT);
		exp[2]=DPState::STRAIGTH, exp[4]=DPState::STRAIGTH, exp[6]=DPState::INVERTED;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

    	stateseq.push_back(new DPState());
    	stateseq.back()->Take(DPState::SHIFT, stateseq, true, 3); // shift-3
    	stateseq.back()->Take(DPState::SHIFT, stateseq, true);
    	stateseq.back()->Take(DPState::INVERTED, stateseq, true);
    	// for a complete tree
    	DPState * goal = stateseq.back();
    	if (!goal->IsGold()){
    		cerr << *goal << endl;
    		ret = 0;
    	}
    	ActionVector act;
    	goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
    	if (!ret){
    		cerr << "incorrect all actions" << endl;
    		ret = 0;
    	}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=3, exp[1]=0, exp[2]=1, exp[3]=2;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}

    	// for an incomplete tree
    	act.clear();
    	goal = stateseq[2];
    	goal->AllActions(act);
    	if (act.size() != 2*n-2){
    		cerr << "incomplete all actions: size " << act.size() << endl;
    		ret = 0;
    	}
    	BOOST_FOREACH(DPState * state, stateseq)
    		delete state;
    	return ret;
    }

    int TestDelete() {
    	// Create a combined alignment
		//  .....
		//  x....
		//  ....x
    	//  ..x..
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,5));
    	al.AddAlignment(MakePair(1,0));
    	al.AddAlignment(MakePair(2,4));
    	al.AddAlignment(MakePair(3,2));
    	CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
    	if (cal.GetSpans()[0].first != -1){
    		cerr << "CombinedAlign fails " << endl;
    		return 0;
    	}
    	IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT));
    	FeatureDataSequence sent;
    	// Create a sentence
    	string str = "1 2 3 4";
    	sent.FromString(str);
    	int n = sent.GetNumWords();
    	ActionVector refseq = ranks.GetReference(&cal);
		ActionVector exp(2*n-1, DPState::SHIFT);
		exp[2]=DPState::DELETE_L; exp[5]=DPState::INVERTED; exp[6]=DPState::STRAIGTH;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(1,1);
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
		ActionVector act;
		goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
		if (!ret){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1); // 0 is deleted
			exp[0]=1, exp[1]=3, exp[2]=2;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
    	return ret;
    }

    int TestInsert() {
    	string str = "1 2 3 4";
    	sent.FromString(str);
    	int n = sent.GetNumWords();
    	ActionVector refseq = DPState::ActionFromString("F F Z V F F V I S E");
		IParser p(2,1);
		int ret=1;
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
		ActionVector act;
		goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
		if (!ret){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1+2); // 0 is deleted, two target words are inserted
			exp[0]=1, exp[1]=-1, exp[2]=3, exp[3]=-1, exp[4]=2;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
    	return ret;
    }

    int TestIntersection() {
    	sent.FromString("a man plays piano");
    	int n = sent.GetNumWords();
    	ActionVector refseq = DPState::ActionFromString("F F Z F F I S E"); // one more idle state
		IParser f2e(1,1);
		int ret=1;
		DPState * gf2e = f2e.GuidedSearch(refseq, n);
		if (!gf2e->IsGold()){
			cerr << *gf2e << endl;
			ret = 0;
		}
		sent.FromString("man NOM piano ACC plays");
		n = sent.GetNumWords();
		refseq = DPState::ActionFromString("F F X F F X F I S E E"); // two more idle states
		IParser e2f(1,2);
		DPState * ge2f = e2f.GuidedSearch(refseq, n);
		if (!ge2f->IsGold()){
			cerr << *ge2f << endl;
			ret = 0;
		}
		refseq = DPState::ActionFromString("F F Z V F F V I S"); // no idle state
		ActionVector act;
		Intersect(act, gf2e, ge2f);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
		if (!ret){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		sent.FromString("a man plays piano");
		n = sent.GetNumWords();
		IParser p(2,1);
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1+2); // 0 is deleted, two target words are inserted
			exp[0]=1, exp[1]=-1, exp[2]=3, exp[3]=-1, exp[4]=2;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
    	return ret;
    }
    int TestInsideOut() {
    	// Create a combined alignment
		//  .x..
		//  ...x
		//  x...
    	//  ..x.
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,4));
    	al.AddAlignment(MakePair(0,1));
    	al.AddAlignment(MakePair(1,3));
    	al.AddAlignment(MakePair(2,0));
    	al.AddAlignment(MakePair(3,2));
    	Ranks cal;
    	FeatureDataSequence sent;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "1 2 3 4";
    	sent.FromString(str);
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
		ActionVector exp(2*n+1, DPState::SHIFT);
		exp[3]=DPState::SWAP; exp[4]=DPState::INVERTED;
		exp[7]=DPState::INVERTED; exp[8]=DPState::STRAIGTH;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		DParser p(1);
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
		ActionVector act;
		goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    		ret = 0;
    	}
    	ret *= CheckVector(refseq, act);
		if (!ret){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}

    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=2, exp[1]=0, exp[2]=3, exp[3]=1;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
    	return ret;
    }

    int TestInsideOutSearch() {
    	// Create a combined alignment
		//  .x..
		//  ...x
		//  x...
    	//  ..x.
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,4));
    	al.AddAlignment(MakePair(0,1));
    	al.AddAlignment(MakePair(1,3));
    	al.AddAlignment(MakePair(2,0));
    	al.AddAlignment(MakePair(3,2));
    	Ranks cal;
    	FeatureDataSequence sent;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "1 2 3 4";
    	sent.FromString(str);
    	int n = sent.GetNumWords();
    	ActionVector refseq = cal.GetReference();
		ActionVector exp(2*n+1, DPState::SHIFT);
		exp[3]=DPState::SWAP; exp[4]=DPState::INVERTED;
		exp[7]=DPState::INVERTED; exp[8]=DPState::STRAIGTH;

    	ShiftReduceModel model;
    	model.SetMaxState(3);
    	FeatureSet set;
        set.AddFeatureGenerator(new FeatureSequence);
        Sentence datas;
        datas.push_back(&sent);
    	DParser p(1);
    	p.Search(model, set, datas);
    	vector<Parser::Result> kbest;
    	p.GetKbestResult(kbest);
    	int ret = 1;
    	for (int k = 0 ; k < kbest.size() ; k++){
    		DPState * goal = p.GetKthBest(k);
    		if (goal->GetStep() != refseq.size()){
        		cerr << "Goal step " << *goal << " != " << refseq.size() << endl;
        		ret = 0;
    		}
    		ActionVector act;
    		goal->AllActions(act);
    		if (act.size() != refseq.size()){
    			cerr << "incomplete all actions: size " << act.size() << " != " << refseq.size() << endl;
    			ret = 0;
    			break;
    		}
    	}
    	return ret;
    }

    int TestSwapAfterReduce(){
    	int n = 5;
    	int ret = 1;
    	// 3 0 4 1 2
    	ActionVector exp = DPState::ActionFromString("F F F S F D I F F I S");
    	DParser p(1);
    	DPState * goal = p.GuidedSearch(exp, n);
		{
		DPState * rchild = goal->RightChild();
		ActionVector act;
		rchild->InsideActions(act);
		if (act.size() != 3){
			ret = 0;
			cerr << "rchild action size " << act.size() << " != 3" << endl;
		}
		}
		ActionVector act;
		goal->AllActions(act);
		if (goal->GetStep() != act.size()){
			ret = 0;
			cerr << "step " << goal->GetStep() << " != action size " << act.size() << endl;
		}
		ret *= CheckVector(exp, act);

    	{ // check reordering
    		vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=3, exp[1]=0, exp[2]=4, exp[3]=1, exp[4]=2;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
		return ret;
    }

    int TestInsideOut2(){
    	int n = 7;
    	int ret = 1;
    	// 1 2 4 0 5 3 6
    	ActionVector exp = DPState::ActionFromString("F F F S F F D S I F F I S F S");
    	DParser p(1);
    	DPState * goal = p.GuidedSearch(exp, n);
    	{ // check reordering

	    	Parser::Result result;
	    	SetResult(&result, goal);
    		vector<int> & act = result.order;
			vector<int> exp(n);
			exp[0]=1, exp[1]=2, exp[2]=4, exp[3]=0;
			exp[4]=5, exp[5]=3, exp[6]=6;
			ret *= CheckVector(exp, act);
			if (!ret){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
		return ret;
    }
    int TestAllow(){
    	string str = "1 2 3 4";
		FeatureDataSequence sent;
		sent.FromString(str);
		int n = sent.GetNumWords();
		int ret = 1;
    	DPStateVector stateseq;
		stateseq.push_back(new DDPState());
		DPState * state = stateseq.back();
		state->Take(DPState::SHIFT, stateseq, true);	// 1
		state = stateseq.back();
		state->Take(DPState::SHIFT, stateseq, true);	// 1 2
		state = stateseq.back();
		if (state->Allow(DPState::SWAP, n)){
			ret = 0;
			cerr << "action " << (char)DPState::SWAP << " is not allowed at step " << state->GetStep() << endl;
		}
		state->Take(DPState::SHIFT, stateseq, true);	// 1 2 3
		state = stateseq.back();
		state->Take(DPState::SWAP, stateseq, true);		// 1 3
		state = stateseq.back();
		if (state->Allow(DPState::SHIFT, n)){
			ret = 0;
			cerr << "action " << (char)DPState::SHIFT << " is not allowed at step " << state->GetStep() << endl;
		}
		state->Take(DPState::STRAIGTH, stateseq, true);		// 1-3
		state = stateseq.back();
		if (state->Allow(DPState::STRAIGTH, n)){
			ret = 0;
			cerr << "action " << (char)DPState::STRAIGTH << " is not allowed at step " << state->GetStep() << endl;
		}
		BOOST_FOREACH(DPState * state, stateseq)
			delete state;

		return ret;
    }

    int TestAllowConsecutiveSwap(){
    	int n = 9;
		ActionVector exp = DPState::ActionFromString("F F F F I F F F S D D D S F F F S F I");
		int ret = 1;
		DParser p(3);
		DPState * goal = p.GuidedSearch(exp, n);
		ActionVector act;
		goal->AllActions(act);
		if (goal->GetStep() != act.size()){
			ret = 0;
			cerr << "step " << goal->GetStep() << " != action size " << act.size() << endl;
		}
		ret *= CheckVector(exp, act);
		return ret;
    }

    int TestTakeAfterMerge(){
    	int n = 10;
		int ret = 1;
    	// this transition sequence represents: 0-4, 5-6, 7, 8
		DParser p1(1);
		ActionVector t1 = DPState::ActionFromString("F F S F F D S F F S S F F S F F");
		DPState * s1 = p1.GuidedSearch(t1, n);
		// this transition sequence represents: 0-1:4, 2-3:5-6, 7, 8
		DParser p2(1);
		ActionVector t2 = DPState::ActionFromString("F F S F F I F D S F F I F S F F");
		DPState * s2 = p2.GuidedSearch(t2, n);
		if (!(*s1 == *s2)){
			ret = 0;
			cerr << *s1 << " != " << *s2 << endl;
		}
		// push the left state of s2 to that of s1
		s1->MergeWith(s2);
		if (s1->GetLeftPtrs().size() != 2){
			ret = 0;
			cerr << "left ptr size " << s1->GetLeftPtrs().size() << " != 2 " << endl;
		}
		DPStateVector result;
		if (!s1->Allow(DPState::SWAP, n)){
			ret = 0;
			cerr << "action " << (char)DPState::SWAP << " is not allowed at step " << s1->GetStep() << endl;
		}
		s1->Take(DPState::SWAP, result, true);
		if (result.size() != 2-1){ //
			ret = 0;
			cerr << "action " << (char)DPState::SWAP << " result size " << result.size() << " != 1 " << endl;
		}
		BOOST_FOREACH(DPState * state, result){
			if (!state->GetLeftState() || !state->GetLeftState()->IsContinuous()){
				ret = 0;
				cerr << "incorrect left state " << endl;
			}
			delete state;
		}
		return ret;
    }

    int TestGetReference(){
    	int ret = 1;
    	Ranks ranks = Ranks::FromString("1 6 4 2 0 5 3 7");
    	ActionVector exp = DPState::ActionFromString("F F F F D D S F F F D D I F F F S I F I S F S");
    	ActionVector refseq = ranks.GetReference();
    	if (!CheckVector(exp, refseq))
    		ret = 0;
    	DParser p1(4);
    	DPState * goal = p1.GuidedSearch(refseq, ranks.GetSrcLen());
    	ActionVector act;
		goal->AllActions(act);
		if (goal->GetStep() != act.size()){
			ret = 0;
			cerr << "step " << goal->GetStep() << " != action size " << act.size() << endl;
		}
		ret *= CheckVector(exp, act);

		ranks = Ranks::FromString("2 6 8 3 5 4 0 1 9 7");
		exp = DPState::ActionFromString("F F F F F F I S D I S F F F S D I F F S F I S");
		refseq = ranks.GetReference();
		if (!CheckVector(exp, refseq))
			ret = 0;
		DParser p2(2);
    	goal = p2.GuidedSearch(refseq, ranks.GetSrcLen());
    	if (!goal){
    		ret = 0;
    		cerr << "fail to get the guided search result " << endl;
    	}
    	act.clear();
		goal->AllActions(act);
		if (goal->GetStep() != act.size()){
			ret = 0;
			cerr << "step " << goal->GetStep() << " != action size " << act.size() << endl;
		}
		ret *= CheckVector(exp, act);

		ranks = Ranks::FromString("0 1 4 8 2 7 3 6 5");
		refseq = ranks.GetReference(); // fails to get correct reference in this scheme
		DParser p3(ranks.GetSrcLen());
    	goal = p3.GuidedSearch(refseq, ranks.GetSrcLen());
    	if (goal && goal->Allow(DPState::IDLE, ranks.GetSrcLen())){
    		ret = 0;
    		goal->PrintTrace(cerr);
    	}

		ranks = Ranks::FromString("0 0 0 0 1 1 2 2 9 9 9 9 9 3 4 10 10 10 10 10 5 11 11 11 11 11 6 7 7 8 12 12");
		refseq = ranks.GetReference(); // fails to get correct reference in this scheme
    	DParser p4(ranks.GetSrcLen());
    	goal = p4.GuidedSearch(refseq, ranks.GetSrcLen());
    	if (goal && goal->Allow(DPState::IDLE, ranks.GetSrcLen())){
    		ret = 0;
    		goal->PrintTrace(cerr);
    	}
		return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestGetReordering()" << endl; if(TestGetReordering()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSetSignature()" << endl; if(TestSetSignature()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestAllActions()" << endl; if(TestAllActions()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSearch()" << endl; if(TestSearch()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSearch2()" << endl; if(TestSearch2()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestShift2()" << endl; if(TestShift2()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestShift3()" << endl; if(TestShift3()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestDelete()" << endl; if(TestDelete()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestInsert()" << endl; if(TestInsert()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestIntersection()" << endl; if(TestIntersection()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestInsideOut()" << endl; if(TestInsideOut()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestInsideOutSearch()" << endl; if(TestInsideOutSearch()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSwapAfterReduce()" << endl; if(TestSwapAfterReduce()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestInsideOut2()" << endl; if(TestInsideOut2()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestAllow()" << endl; if(TestAllow()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestAllowConsecutiveSwap()" << endl; if(TestAllowConsecutiveSwap()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestTakeAfterMerge()" << endl; if(TestTakeAfterMerge()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestGetReference()" << endl; if(TestGetReference()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestShiftReduceParser Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent;
};
}
#endif /* TEST_SHIFT_REDUCE_PARSER_H_ */
