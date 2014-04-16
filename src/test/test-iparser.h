/*
 * test-iparser.h
 *
 *  Created on: Apr 14, 2014
 *      Author: leona
 */

#ifndef TEST_IPARSER_H_
#define TEST_IPARSER_H_

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

class TestIParser : public TestBase {

public:

    TestIParser(){
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
    ~TestIParser() { }

    int TestInsertRank(){
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
    	CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS);
    	if (cal.GetSpans()[0].first != -1){
    		cerr << "CombinedAlign fails " << endl;
    		return 0;
    	}
    	IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
    						CombinedAlign::ATTACH_NULL_LEFT);							// attach null in target
    	ranks.Insert(&cal);
    	int ret = 1;
    	if (ranks.GetRanks().size() != cal.GetSpans().size()){
    		cerr << "incorrect insert: size " << ranks.GetRanks().size() << " != " << cal.GetSpans().size() << endl;
    		ret = 0;
    	}
    	vector<int> expr(words.size()+2); // two words are inserted
    	expr[0]=0, expr[1]=0, expr[2]=Ranks::INSERT_R;
    	expr[3]=2, expr[4]=1, expr[5]=Ranks::INSERT_R;
    	if (!CheckVector(expr, ranks.GetRanks())){
    		cerr << "incorrect insert" << endl;
    		ret = 0;
    	}
    	vector<CombinedAlign::Span> exps(words.size()+2); // two words are inserted
		exps[0]=MakePair(-1,-1), exps[1]=MakePair(0,0), exps[2]=MakePair(1,1);
		exps[3]=MakePair(4,4), exps[4]=MakePair(2,2), exps[5]=MakePair(3,3);
		if (!CheckVector(exps, cal.GetSpans())){
			cerr << "incorrect insert" << endl;
			ret = 0;
		}
    	return ret;
    }
    int TestGetReference() {
    	// Create a combined alignment
		//  .....
		//  x....
		//  ....x
    	//  ..x..
    	//	"a man plays piano"
    	//	"man NOM piano ACC plays"
    	vector<string> words(4, "x");
    	Alignment al(MakePair(4,5));
    	al.AddAlignment(MakePair(1,0));
    	al.AddAlignment(MakePair(2,4));
    	al.AddAlignment(MakePair(3,2));
    	CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
    	if (cal.GetSpans()[0] != CombinedAlign::Span(-1,-1)){
    		cerr << "CombinedAlign fails " << endl;
    		return 0;
    	}
    	IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
    						CombinedAlign::ATTACH_NULL_LEFT);							// attach null in target
    	ranks.Insert(&cal); // for insert
    	ActionVector refseq = ranks.GetReference(&cal);

    	int n = words.size();
		int ret = 1;
    	if (ranks.GetRanks().size() != cal.GetSpans().size()){
    		cerr << "incorrect get reference: size " << ranks.GetRanks().size() << " != " << cal.GetSpans().size() << endl;
    		ret = 0;
    	}
		ActionVector exp(2*n-1+2, DPState::SHIFT); // two words are inserted
		exp[2]=DPState::DELETE_L; exp[3]=DPState::INSERT_R; exp[6]=DPState::INSERT_R;
		exp[7]=DPState::INVERTED; exp[8]=DPState::STRAIGTH;
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(2,1);
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
		ActionVector act;
		goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << refseq.size() << " != " << act.size() << endl;
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
			vector<int> exp(n-1+2);
			exp[0]=1, exp[1]=-1, exp[2]=3, exp[3]=-1, exp[4]=2;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
    	}
    	return ret;
    }

    int TestMergeMismatch(){
    	sent.FromString("I love her and the details are unimportant .");
    	Alignment al = Alignment::FromString("9-15 ||| 0-0 2-2 1-4 3-5 4-6 5-7 7-9 7-10 7-11 7-12 8-13 8-14");
    	const vector<string> & words = sent.GetSequence();
    	CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
    	if (cal.GetSpans()[6] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails " << endl;
			return 0;
		}
    	IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
    	    						CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
    	ranks.Insert(&cal); // for insert
    	ActionVector refseq = ranks.GetReference(&cal);
    	int n = words.size();
    	int ret = 1;
    	ActionVector exp = DPState::ActionFromString("F V F F V I S F S F S F S V F F Z S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(3,1);
		DPState * goal = p.GuidedSearch(refseq, n);
		if (!goal->IsGold()){
			cerr << *goal << endl;
			ret = 0;
		}
		ActionVector act;
		goal->AllActions(act);
    	if (act.size() != refseq.size()){
    		cerr << "incomplete all actions: size " << refseq.size() << " != " << act.size() << endl;
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
    		vector<int> exp(n-1+3); // one source word is deleted, three target words are inserted
    		exp[0]=0, exp[1]=-1, exp[2]=2, exp[3]=-1, exp[4]=1;
    		exp[5]=3, exp[6]=4, exp[7]=5, exp[8]=-1, exp[9]=7, exp[10]=8;
    		if (!CheckVector(exp, act)){
    			cerr << "incorrect get reordering" << endl;
    			ret = 0;
    		}
    	}
    	return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestInertRank()" << endl; if(TestInsertRank()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestGetReference()" << endl; if(TestGetReference()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestMergeMismatch()" << endl; if(TestMergeMismatch()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestIParser Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent;
};
}


#endif /* TEST_IPARSER_H_ */
