/*
 * test-flat-tree.h
 *
 *  Created on: Feb 19, 2014
 *      Author: leona
 */

#ifndef TEST_FLAT_TREE_H_
#define TEST_FLAT_TREE_H_

#include "test-base.h"
#include <lader/combined-alignment.h>
#include <lader/feature-sequence.h>
#include <lader/feature-data-sequence.h>
#include <lader/feature-set.h>
#include <reranker/flat-tree.h>
#include <reranker/features.h>
#include <reranker/read-tree.h>
#include <fstream>
#include <vector>
using namespace reranker;

namespace lader {

class TestFlatTree : public TestBase {

public:

    TestFlatTree(){
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
			return;
		}

		stateseq.push_back(new DPState());
		for (int step = 1 ; step < 2*n ; step++){
			DPState * state = stateseq.back();
			state->Take(refseq[step-1], stateseq, true);
		}
		// for a complete tree
		goal = stateseq.back();
		if (!goal->IsGold()){
			cerr << *goal << endl;
			return;
		}
    }
    ~TestFlatTree() { }

    int TestFlatten() {
    	int ret = 1;
		int n = sent.GetNumWords();
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
    	int ret = 1;
		int n = sent.GetNumWords();
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


    int TestParseInput(){
    	int ret = 1;
    	string line = "(I (S (F this) (F has) (F spurious)) (F ambiguity))";
    	GenericNode dummy('R');
    	GenericNode * root = ParseInput(line);
    	dummy.AddChild(root);
    	ostringstream oss;
    	root->PrintParse(sent.GetSequence(), oss);
    	if (line != oss.str()){
    		cerr << "ParseInput fails: " << oss.str() << endl;
    		ret = 0;
    	}
    	return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestFlatten()" << endl; if(TestFlatten()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestFeatures()" << endl; if(TestFeatures()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestParseInput()" << endl; if(TestParseInput()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestFlatTree Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent;
    DPState * goal;
};
}
#endif /* TEST_FLAT_TREE_H_ */
