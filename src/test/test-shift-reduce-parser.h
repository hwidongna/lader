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
#include <reranker/flat-tree.h>
#include <reranker/features.h>
#include <fstream>
#include <vector>
using namespace reranker;

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
    	FeatureDataSequence sent, sent_pos;
    	cal = Ranks(CombinedAlign(words,al));
    	// Create a sentence
    	string str = "let's shift 3 words";
    	sent.FromString(str);
    	DPStateVector stateseq;
    	int n = sent.GetNumWords();
    	vector<DPState::Action> refseq = cal.GetReference();
		vector<DPState::Action> exp(2*n-1, DPState::SHIFT);
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
			exp[0]=3, exp[1]=0, exp[2]=1, exp[3]=2;
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

    int TestFlatten() {
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
		FeatureDataSequence sent, sent_pos;
		cal = Ranks(CombinedAlign(words,al));
		// Create a sentence
		string str = "this has spurious ambiguity";
		sent.FromString(str);
		DPStateVector stateseq;
		int n = sent.GetNumWords();
		vector<DPState::Action> refseq = cal.GetReference();
		vector<DPState::Action> exp(2*n-1, DPState::SHIFT);
		exp[2]=DPState::STRAIGTH, exp[4]=DPState::STRAIGTH, exp[6]=DPState::INVERTED;
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
    	DPStateNode dummy(0, n, NULL, DPState::INIT);
    	DPStateNode * root = dummy.Flatten(goal);
    	NodeList & children = root->GetChildren();
    	if ( children.size() != 2 ){
    		cerr << "root node has " << children.size() << " children != 2" << endl;
    		ret = 0;
    	}
    	if ( root->GetLabel() != (char)DPState::INVERTED ){
    		cerr << "root node " << root->GetAction() << " != " << (char)DPState::INVERTED << endl;
    		return 0;
    	}

    	Node * lchild = children.front();
    	if ( lchild->GetLabel() != (char)DPState::STRAIGTH ){
    		cerr << "lchild " << lchild->GetLabel() << " != " << (char)DPState::STRAIGTH << endl;
    		ret = 0;
    	}
    	if ( lchild->GetParent() != root ){
    		cerr << "incorrect lchild.parent " << *lchild->GetParent() << " != " << *root << endl;
    		ret = 0;
    	}
    	if ( lchild->GetChildren().size() != 3 ){
			cerr << "lchild has " << lchild->GetChildren().size() << " != 3 children" << endl;
			ret = 0;
		}
    	BOOST_FOREACH(Node * node, lchild->GetChildren()){
    		if ( node->GetLabel() != (char)DPState::SHIFT ){
				cerr << "node " << node->GetLabel() << " != " << (char)DPState::SHIFT << endl;
				ret = 0;
			}
    		if ( node->GetParent() != lchild ){
    			cerr << "incorrect node.parent " << *node->GetParent() << " != " << *lchild << endl;
    			ret = 0;
    		}
    	}
    	Node * rchild = children.back();
    	if ( rchild->GetLabel() != (char)DPState::SHIFT ){
    		cerr << "rchild " << rchild->GetLabel() << " != " << (char)DPState::SHIFT << endl;
    		ret = 0;
    	}
    	if ( rchild->GetParent() != root ){
    		cerr << "incorrect rchild.parent " << *rchild->GetParent() << " != " << *root << endl;
    		ret = 0;
    	}
    	return ret;
    }

    int TestFeatures() {
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
		FeatureDataSequence sent, sent_pos;
		cal = Ranks(CombinedAlign(words,al));
		// Create a sentence
		string str = "this has spurious ambiguity";
		sent.FromString(str);
		DPStateVector stateseq;
		int n = sent.GetNumWords();
		vector<DPState::Action> refseq = cal.GetReference();
		vector<DPState::Action> exp(2*n-1, DPState::SHIFT);
		exp[2]=DPState::STRAIGTH, exp[4]=DPState::STRAIGTH, exp[6]=DPState::INVERTED;
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
    	DPStateNode dummy(0, n, NULL, DPState::INIT);
    	DPStateNode * root = dummy.Flatten(goal);
    	Rule rule;
    	rule.Extract(root, sent);
    	if (rule.FeatureName() != "Rule:I-S.F"){
    		cerr << "incorrect rule feature " << rule.FeatureName() << endl;
    		ret = 0;
    	}
    	Rule parentlexrule;
    	parentlexrule.SetLexicalized(true, false);
    	parentlexrule.Extract(root, sent);
    	if (parentlexrule.FeatureName() != "Rule:I(this,ambiguity)-S.F"){
    		cerr << "incorrect parent lexicalized rule feature " << parentlexrule.FeatureName() << endl;
    		ret = 0;
    	}

    	Rule childlexrule;
		childlexrule.SetLexicalized(false, true);
		childlexrule.Extract(root, sent);
		if (childlexrule.FeatureName() != "Rule:I-S(this,spurious).F(ambiguity,ambiguity)"){
			cerr << "incorrect child lexicalized rule feature " << childlexrule.FeatureName() << endl;
			ret = 0;
		}

		Word word(3);
		word.Extract(root, sent);
		word.GetFeature();
		vector<string> word_exp(4);
		word_exp[0] = "(this)/S/I"; word_exp[1]="(has)/S/I";
		word_exp[2]="(spurious)/S/I"; word_exp[3]="(ambiguity)/I/R";
		ret = CheckVector(word_exp, word.GetFeature());
		if (!ret)
			cerr << "incorrect word feature " << endl;

    	NGram ngram(2);
    	ngram.Extract(root, sent);
    	vector<string> ngram_exp(3);
    	ngram_exp[0] = "I-[.S"; ngram_exp[1]="I-S.F"; ngram_exp[2]="I-F.]";
    	ret = CheckVector(ngram_exp, ngram.GetFeature());
    	if (!ret)
    		cerr << "incorrect ngram feature " << ngram.FeatureName() << endl;

    	NodeList terminals;
    	root->GetTerminals(terminals);
    	if (terminals.size() != sent.GetNumWords()){
    		cerr << "incorrect # of terminals " << terminals.size() << " != " << sent.GetNumWords() << endl;
    		ret = 0;
    	}
    	if (root->size() != sent.GetNumWords()){
    		cerr << "incorect size of root " << root->size() << " != " << sent.GetNumWords() << endl;
    		ret = 0;
    	}
    	NGramTree ngramtree(2);
    	ngramtree.Extract(root, sent);
    	vector<string> ngramtree_exp(3);
    	ngramtree_exp[0] = "(S (F this) (F has))";
    	ngramtree_exp[1] = "(S (F has) (F spurious))";
    	ngramtree_exp[2] = "(I (S (F spurious)) (F ambiguity))";
    	ret = CheckVector(ngramtree_exp, ngramtree.GetFeature());
    	if (!ret)
    		cerr << "incorrect ngramtree feature " << ngramtree.FeatureName() << endl;

    	return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestGetReordering()" << endl; if(TestGetReordering()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSetSignature()" << endl; if(TestSetSignature()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestAllActions()" << endl; if(TestAllActions()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestShift2()" << endl; if(TestShift2()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestShift3()" << endl; if(TestShift3()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestFlatten()" << endl; if(TestFlatten()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestFeatures()" << endl; if(TestFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestShiftReduceParser Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent, sent_pos;
};
}
#endif /* TEST_SHIFT_REDUCE_PARSER_H_ */
