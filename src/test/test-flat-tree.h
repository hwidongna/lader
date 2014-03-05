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
#include <shift-reduce-dp/ddpstate.h>
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
    	NodeList children = root->GetChildren();
    	if ( children.size() != 2 ){
    		cerr << "root node has " << children.size() << " children != 2" << endl;
    		ret = 0;
    	}
    	if ( root->GetLabel() != (char)DPState::INVERTED ){
    		cerr << "root node " << root->GetLabel() << " != " << (char)DPState::INVERTED << endl;
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
    int TestFlattenInsideOut() {
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
    	vector<DPState::Action> refseq = cal.GetReference(1);
		vector<DPState::Action> exp(2*n+1, DPState::SHIFT);
		exp[3]=DPState::SWAP; exp[4]=DPState::INVERTED;
		exp[7]=DPState::INVERTED; exp[8]=DPState::STRAIGTH;
		int ret = 1;
		ret *= CheckVector(exp, refseq);
		if (!ret){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

    	DPStateVector stateseq;
		stateseq.push_back(new DDPState());
		for (int i = 0 ; i < exp.size() ; i++){
			DPState * state = stateseq.back();
			if (state->Allow(exp[i], n))
				state->Take(exp[i], stateseq, true);
			else{
				ret = 0;
				cerr << "action " << (char)exp[i] << "is not allowed at step " << i+1 << endl;
				break;
			}
		}
		DPState * goal = stateseq.back();
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
    	DDPStateNode dummy(0, n, NULL, DPState::INIT);
    	DPStateNode * root = dummy.Flatten(goal);
    	NodeList children = root->GetChildren();
    	if ( children.size() != 2 ){
    		cerr << "root node has " << children.size() << " children != 2" << endl;
    		ret = 0;
    	}
    	if ( root->GetLabel() != (char)DPState::STRAIGTH ){
    		cerr << "root node " << root->GetLabel() << " != " << (char)DPState::STRAIGTH << endl;
    		return 0;
    	}

    	Node * lchild = children.front();
    	if ( lchild->GetLabel() != (char)DPState::INVERTED ){
    		cerr << "lchild " << lchild->GetLabel() << " != " << (char)DPState::INVERTED << endl;
    		ret = 0;
    	}
    	if ( lchild->GetParent() != root ){
    		cerr << "incorrect lchild.parent " << *lchild->GetParent() << " != " << *root << endl;
    		ret = 0;
    	}
    	if ( lchild->GetChildren().size() != 2 ){
			cerr << "lchild has " << lchild->GetChildren().size() << " != 2 children" << endl;
			ret = 0;
		}
    	Node * lrchild = lchild->GetChildren().back();
    	if ( lrchild->GetLabel() != (char)DPState::SWAP ){
    		cerr << "lrchild " << lrchild->GetLabel() << " != " << (char)DPState::SWAP << endl;
    		ret = 0;
    	}
    	Node * rchild = children.back();
    	if ( rchild->GetLabel() != (char)DPState::INVERTED ){
    		cerr << "rchild " << rchild->GetLabel() << " != " << (char)DPState::INVERTED << endl;
    		ret = 0;
    	}
    	if ( rchild->GetParent() != root ){
    		cerr << "incorrect rchild.parent " << *rchild->GetParent() << " != " << *root << endl;
    		ret = 0;
    	}
    	if ( rchild->GetChildren().size() != 2 ){
    		cerr << "lchild has " << rchild->GetChildren().size() << " != 2 children" << endl;
    		ret = 0;
    	}
    	return ret;
    }
    int TestExtract() {
    	int ret = 1;
		int n = sent.GetNumWords();
		DPStateNode dummy(0, n, NULL, DPState::INIT);
    	DPStateNode * root = dummy.Flatten(goal);

    	Rule rule;
    	rule.Extract(root, sent);
    	vector<string> rule_exp(2);
    	// postorder traversal
    	rule_exp[0] = "S-F.F.F";
    	rule_exp[1] = "I-S.F";
    	ret = CheckVector(rule_exp, rule.GetFeature());
    	if (!ret)
    		cerr << "incorrect rule feature" << endl;

    	Rule parentlexrule;
    	parentlexrule.SetLexical(true, false);
    	parentlexrule.Extract(root, sent);
    	vector<string> parent_exp(2);
    	parent_exp[0] = "S(this,spurious)-F.F.F";
    	parent_exp[1] = "I(this,ambiguity)-S.F";
    	ret = CheckVector(parent_exp, parentlexrule.GetFeature());
    	if (!ret)
    		cerr << "incorrect parent lexicalized rule" << endl;

    	Rule childlexrule;
		childlexrule.SetLexical(false, true);
		childlexrule.Extract(root, sent);
		vector<string> child_exp(2);
		child_exp[0] = "S-F(this,this).F(has,has).F(spurious,spurious)";
		child_exp[1] = "I-S(this,spurious).F(ambiguity,ambiguity)";
    	ret = CheckVector(child_exp, childlexrule.GetFeature());
		if (!ret)
			cerr << "incorrect child lexicalized rule" << childlexrule.FeatureName() << endl;

		Word word(3);
		word.Extract(root, sent);
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

    	cNodeList terminals;
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
    	GenericNode::ParseResult * result = GenericNode::ParseInput(line);
    	ostringstream oss;
    	result->root->PrintParse(sent.GetSequence(), oss);
    	if (line != oss.str()){
    		cerr << "ParseInput fails: " << oss.str() << endl;
    		ret = 0;
    	}
    	ret = CheckVector(sent.GetSequence(), result->words);
    	if (!ret){
    		cerr << "ParseWord fails:";
    		BOOST_FOREACH(string word, result->words)
    			cerr << " " << word;
    		cerr << endl;
    	}

    	string iline = "(I (S (F 0) (F 1) (F 2)) (F 3))";
    	oss.seekp(0);
    	result->root->PrintParse(oss);
    	oss << std::ends;
    	if (iline != oss.str().data()){
			cerr << "ParseInput fails: " << oss.str().data() << endl;
			ret = 0;
		}
    	delete result->root;
    	delete result;
    	return ret;
    }

    int TestSet(){
		int ret = 1;
		string line = "(I (S (F this) (F has) (F spurious)) (F ambiguity))";
		GenericNode::ParseResult * result = GenericNode::ParseInput(line);

		Rule rule;
		rule.Extract(result->root, sent);
    	vector<string> rule_exp(2);
    	// postorder traversal
    	rule_exp[0] = "S-F.F.F";
    	rule_exp[1] = "I-S.F";
    	ret = CheckVector(rule_exp, rule.GetFeature());
    	if (!ret)
    		cerr << "incorrect rule feature" << endl;

		FeatureMapInt feat;
		RerankerModel model;
		rule.Set(feat, model);
		if (feat.size() != 2){
			cerr << "fail to set feature map: size " << feat.size() << " != " << 2 << endl;
			ret = 0;
		}
		SymbolSet<int> & symbols = model.GetFeatureIds();
		FeatureVectorInt feat_exp(2);
		feat_exp[0] = MakePair(symbols.GetId("Rule:S-F.F.F"), 1);
		feat_exp[1] = MakePair(symbols.GetId("Rule:I-S.F"), 1);
		BOOST_FOREACH(FeaturePairInt fp, feat_exp){
			FeatureMapInt::iterator it = feat.find(fp.first);
			if (it == feat.end()){
				cerr << "incorrect feature key " << symbols.GetSymbol(fp.first) << endl;
				ret = 0;
			}
			else if (it->second != fp.second){
				cerr << "incorrect feature value " << fp.second << "!=" << it->second << endl;
				ret = 0;
			}
			else if (model.GetCount(it->first) != 1){
				cerr << "incorrect feature count for key " << symbols.GetSymbol(fp.first) << endl;
			}
		}
		delete result->root;
		delete result;
		return ret;
	}

    int TestIntersection(){
    	int ret = 1, count;
		string line1 = "(I (S (F this) (F has) (F spurious)) (F ambiguity))";
		GenericNode::ParseResult * result1 = GenericNode::ParseInput(line1);
		cNodeList nonterminals;
		result1->root->GetNonTerminals(nonterminals);
		if (nonterminals.size() != result1->root->NumEdges()){
			cerr << "different # nonteriminals: " << nonterminals.size() << " != " << result1->root->NumEdges() << endl;
			ret = 0;
		}
		count = Node::Intersection(result1->root, result1->root);
		if (count != result1->root->NumEdges()){
			cerr << "incorrect intersection: " << count << " != " << result1->root->NumEdges();
			ret= 0;
		}
		string line2 = "(S (S (F this) (F has) (F spurious)) (F ambiguity))"; // one label is different
		GenericNode::ParseResult * result2 = GenericNode::ParseInput(line2);
		if (result1->root->NumEdges() != result2->root->NumEdges()){
			cerr << "different # edges: " << result1->root->NumEdges() << " != " << result2->root->NumEdges() << endl;
			ret = 0;
		}
		count = Node::Intersection(result1->root, result2->root);
		if (count != result2->root->NumEdges()-1){
			cerr << "incorrect intersection: " << count << " != " << result2->root->NumEdges()-1 << endl;
			ret= 0;
		}

		string line3 = "(I (S (F this) (I (F has) (F spurious))) (F ambiguity))"; // one more bracket
		GenericNode::ParseResult * result3 = GenericNode::ParseInput(line3);
		count = Node::Intersection(result1->root, result3->root);
		if (count != result3->root->NumEdges()-1){
			cerr << "incorrect intersection: " << count << " != " << result3->root->NumEdges()-1 << endl;;
			ret= 0;
		}
		vector<int> act;
		result3->root->GetOrder(act);
		vector<int> exp(4);
		exp[0]=3; exp[1]=0; exp[2]=2; exp[3]=1;
		ret = CheckVector(exp, act);

		string line4 = "(R (S (F this) (I (F has) (F spurious))) (F ambiguity))"; // one more bracket, one different label
		GenericNode::ParseResult * result4 = GenericNode::ParseInput(line4);
		count = Node::Intersection(result1->root, result4->root);
		if (count != result4->root->NumEdges()-2){
			cerr << "incorrect intersection: " << count << " != " << result4->root->NumEdges()-2 << endl;;
			ret= 0;
		}

		act.clear();
		result4->root->GetOrder(act);
		exp[0]=0; exp[1]=2; exp[2]=1; exp[3]=3;
		ret = CheckVector(exp, act);
		return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestFlatten()" << endl; if(TestFlatten()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestFlattenInsideOut()" << endl; if(TestFlattenInsideOut()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestExtract()" << endl; if(TestExtract()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestParseInput()" << endl; if(TestParseInput()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestSet()" << endl; if(TestSet()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestIntersection()" << endl; if(TestIntersection()) succeeded++; else cout << "FAILED!!!" << endl;
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
