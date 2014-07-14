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
#include <lader/reorderer-runner.h>
#include <shift-reduce-dp/config-iparser-gold.h>
#include <shift-reduce-dp/iparser-gold.h>
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
    	if (cal.GetSpans()[0] != CombinedAlign::Span(-1,-1)){
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
    	if (!CheckVector(Ranks::FromString("0 0 -1 2 1 -1").GetRanks(), ranks.GetRanks())){
    		cerr << "Ranks::Insert fails " << endl;
    		return 0;
    	}
    	ActionVector refseq = ranks.GetReference(&cal);

    	int n = words.size();
		int ret = 1;
    	if (ranks.GetRanks().size() != cal.GetSpans().size()){
    		cerr << "incorrect get reference: size " << ranks.GetRanks().size() << " != " << cal.GetSpans().size() << endl;
    		ret = 0;
    	}
		ActionVector exp = DPState::ActionFromString("F F V Z F F V I S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
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

    int TestMixed(){
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
    	if (!CheckVector(Ranks::FromString("0 -1 2 1 -1 3 4 5 -1 6 6 7").GetRanks(), ranks.GetRanks())){
    		cerr << "Ranks::Insert fails " << endl;
    		return 0;
    	}
    	ActionVector refseq = ranks.GetReference(&cal);
    	int n = words.size();
    	int ret = 1;
    	ActionVector exp = DPState::ActionFromString("F V F F V I S F S F S F V S F F Z S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
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

    int TestInverted(){
    	sent.FromString("In a world that runs on energy");
    	Alignment al = Alignment::FromString("7-7 ||| 6-1 5-2 4-3 3-4 2-5 0-6");
    	const vector<string> & words = sent.GetSequence();
    	CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
    	if (cal.GetSpans()[1] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails " << endl;
			return 0;
		}
    	IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
    	    						CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
    	ranks.Insert(&cal); // for insert
    	if (!CheckVector(Ranks::FromString("5 4 4 3 2 1 -2 0").GetRanks(), ranks.GetRanks())){
    		cerr << "Ranks::Insert fails " << endl;
    		return 0;
    	}
    	ActionVector refseq = ranks.GetReference(&cal);
    	int n = words.size();
    	int ret = 1;
    	ActionVector exp = DPState::ActionFromString("F F F Z I F I F I F I F C I");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
    	{ // check reordering
    		vector<int> act;
    		goal->GetReordering(act);
    		vector<int> exp(n-1+1); // one source word is deleted, one target word is inserted
    		exp[0]=Ranks::INSERT_L, exp[1]=6, exp[2]=5;
    		exp[3]=4, exp[4]=3, exp[5]=2, exp[6]=0;
    		if (!CheckVector(exp, act)){
    			cerr << "incorrect get reordering" << endl;
    			ret = 0;
    		}
    	}
    	return ret;
    }

    int TestNullSourceMultiple(){
    	sent.FromString("what is the purpose of this notice ?");
		Alignment al = Alignment::FromString("8-6 ||| 5-0 6-1 4-2 3-3 0-5 7-5");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
		if (cal.GetSpans()[1] != CombinedAlign::Span(-1,-1)
		|| cal.GetSpans()[2] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails " << endl;
			return 0;
		}
		IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
									CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
		ranks.Insert(&cal); // for insert
		if (!CheckVector(Ranks::FromString("4 3 3 3 -1 2 0 1 4").GetRanks(), ranks.GetRanks())){
			cerr << "Ranks::Insert fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F F F V Z Z I F I F F S I F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-2+1); // two source words are deleted, one target word is inserted
			exp[0]=5, exp[1]=6, exp[2]=4;
			exp[3]=3, exp[4]=Ranks::INSERT_R, exp[5]=0, exp[6]=7;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
    	return ret;
    }

    int TestNullTargetFront(){
    	sent.FromString("ball NOM her to thrown PASS VEND .");
		Alignment al = Alignment::FromString("8-7 ||| 0-1 5-2 6-2 4-3 3-4 2-5 7-6");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
		if (cal.GetSpans()[1] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails " << endl;
			return 0;
		}
		IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_LEFT),	// attach null in source
									CombinedAlign::ATTACH_NULL_RIGHT);					// attach null in target
		ranks.Insert(&cal); // for insert
		if (!CheckVector(Ranks::FromString("-2 0 0 4 3 2 1 1 5").GetRanks(), ranks.GetRanks())){
			cerr << "Ranks::Insert fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F C F X F F I F I F F S I S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1+1); // one source word is deleted, one target word is inserted
			exp[0]=Ranks::INSERT_L, exp[1]=0, exp[2]=5;
			exp[3]=6, exp[4]=4, exp[5]=3, exp[6]=2, exp[7]=7;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
		return ret;
    }

    int TestNullMixed(){
    	sent.FromString("The oligarchy that controls the region");
		Alignment al = Alignment::FromString("6-9 ||| 5-1 3-3 2-4 0-5 1-5 0-6 1-6 0-7 1-7");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
		if (cal.GetSpans()[4] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails: span[4] = ["
					<< cal.GetSpans()[4].first << ", " << cal.GetSpans()[4].second << "]" << endl;
			return 0;
		}
		IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
									CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
		ranks.Insert(&cal); // for insert
		if (!CheckVector(Ranks::FromString("3 3 2 1 -2 0 0 -1").GetRanks(), ranks.GetRanks())){
			cerr << "Ranks::Insert fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F S F I F I F F V Z C I");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1+2); // one source word is deleted, two target words are inserted
			exp[0]=Ranks::INSERT_L, exp[1]=5, exp[2]=Ranks::INSERT_R;
			exp[3]=3, exp[4]=2, exp[5]=0, exp[6]=1;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
		return ret;
    }

    int TestNullLeftRight(){
    	sent.FromString("the budget director , said the rift within the Republican Party could be an opening for compromise .");
		Alignment al = Alignment::FromString("18-24 ||| 1-0 2-1 3-3 9-4 10-4 7-5 9-5 10-5 7-6 6-9 16-11 15-12 14-13 12-15 11-16 12-16 11-17 11-18 4-19 4-20 4-21 4-22 17-23");
		const vector<string> & words = sent.GetSequence();
		int n = words.size();
		CombinedAlign cal(words,al,
				CombinedAlign::ATTACH_NULL_RIGHT, // no delete
				CombinedAlign::COMBINE_BLOCKS);	// combine blocks
		IParserRanks ranks(CombinedAlign(words,al,
											CombinedAlign::ATTACH_NULL_RIGHT, 			// attach null in source
											CombinedAlign::COMBINE_BLOCKS),				// combine blocks in source
									CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
		ranks.Insert(&cal); // for insert
		if (!CheckVector(Ranks::FromString("0 0 1 -1 2 9 4 4 -1 3 3 3 3 -1 8 8 7 7 -1 6 5 10").GetRanks(), ranks.GetRanks())){
			cerr << "Ranks::Insert fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F S F V S F S F F F V S F F S F S F V S I F F S F F V S I F I F I S I S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		return ret;
	}
    int TestDiscontinuousTarget(){
		sent.FromString("if she ate rice");
		// if(만약) she rice ate if(다면)
		Alignment al = Alignment::FromString("4-5 ||| 0-0 1-1 3-2 2-3 0-4");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al, CombinedAlign::ATTACH_NULL_RIGHT); // no delete
		{
		vector<pair<double,double> > exp(4);
		exp[0] = MakePair(0,4);
		exp[1] = MakePair(1,1);
		exp[2] = MakePair(3,3);
		exp[3] = MakePair(2,2);
		if (!CheckVector(exp, cal.GetSpans())){
			cerr << "CombinedAling fails" << endl;
			return 0;
		}
		}
		IParserRanks ranks(CombinedAlign(words,al,
				CombinedAlign::ATTACH_NULL_RIGHT, 			// attach null in source
				CombinedAlign::COMBINE_BLOCKS),				// combine blocks in source
				CombinedAlign::ATTACH_NULL_LEFT);			// attach null in target
		// TODO additional action and dimension for discontinuity?
//		if (!CheckVector(Ranks::FromString("0 1 3 2 0").GetRanks(), ranks.GetRanks())){
		if (!CheckVector(Ranks::FromString("0 0 0 0").GetRanks(), ranks.GetRanks())){
			cerr << "IParserRanks fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F S F S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n);
			exp[0]=0, exp[1]=1, exp[2]=2, exp[3]=3;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
		return ret;
	}

    int TestDiscontinuousTargetWithNull(){
		sent.FromString("if she ate rice");
		// if(만약) she NOM rice ACC ate if(다면)
		Alignment al = Alignment::FromString("4-7 ||| 0-0 1-1 3-3 2-5 0-6");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al,
				CombinedAlign::ATTACH_NULL_RIGHT,	// no delete
				CombinedAlign::COMBINE_BLOCKS); 	// combine block
		IParserRanks ranks(CombinedAlign(words,al,
				CombinedAlign::ATTACH_NULL_RIGHT, 			// attach null in source
				CombinedAlign::COMBINE_BLOCKS),				// combine blocks in source
				CombinedAlign::ATTACH_NULL_LEFT);			// attach null in target
		ranks.Insert(&cal);
		// TODO ?
		if (!CheckVector(Ranks::FromString("0 0 -1 0 -1 0 -1").GetRanks(), ranks.GetRanks())){
			cerr << "IParserRanks::Insert fails" << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F V S F V S F V S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n+3);
			exp[0]=0, exp[1]=1, exp[2]=-1, exp[3]=3, exp[4]=-1, exp[5]=2, exp[6]=-1;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
		return ret;
	}

    int Test122(){
    	sent.FromString("The oligarchy that controls the region is well-known for their radical views .");
		Alignment al = Alignment::FromString("13-24 ||| 5-1 3-3 2-4 0-5 1-5 0-6 1-6 0-7 1-7 10-9 10-10 11-11 8-15 6-16 7-16 6-17 7-17 6-18 7-18 6-19 7-19 6-20 7-20 6-21 7-21 12-22 12-23");
		const vector<string> & words = sent.GetSequence();
		CombinedAlign cal(words,al, CombinedAlign::LEAVE_NULL_AS_IS); // for delete
		if (cal.GetSpans()[9] != CombinedAlign::Span(-1,-1)){
			cerr << "CombinedAlign fails " << endl;
			return 0;
		}
		IParserRanks ranks(CombinedAlign(words,al, CombinedAlign::ATTACH_NULL_RIGHT),	// attach null in source
									CombinedAlign::ATTACH_NULL_LEFT);					// attach null in target
		ranks.Insert(&cal); // for insert
		if (!CheckVector(Ranks::FromString("3 3 -1 2 1 -2 0 0 -1 7 7 6 4 4 5 -1 8").GetRanks(), ranks.GetRanks())){
			cerr << "Ranks::Insert fails " << endl;
			return 0;
		}
		ActionVector refseq = ranks.GetReference(&cal);
		int n = words.size();
		int ret = 1;
		ActionVector exp = DPState::ActionFromString("F F V S F I F I F C F Z V I F F S F I F F Z F V S I S F S");
		if (!CheckVector(exp, refseq)){
			cerr << "incorrect reference sequence" << endl;
			return 0;
		}

		IParser p(n,n);
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
		if (!CheckVector(refseq, act)){
			cerr << "incorrect all actions" << endl;
			ret = 0;
		}
		{ // check reordering
			vector<int> act;
			goal->GetReordering(act);
			vector<int> exp(n-1+3); // one source word is deleted, one target word is inserted
			exp[0]=Ranks::INSERT_L, exp[1]=5, exp[2]=Ranks::INSERT_R;
			exp[3]=3, exp[4]=2, exp[5]=0, exp[6]=1, exp[7]=Ranks::INSERT_R;
			exp[8]=10, exp[9]=11, exp[10]=Ranks::INSERT_R;
			exp[11]=8, exp[12]=6, exp[13]=7, exp[14]=12;
			if (!CheckVector(exp, act)){
				cerr << "incorrect get reordering" << endl;
				ret = 0;
			}
		}
		return ret;
    }

    int TestGoldOutput(){
//    	string sline = "An apartment complex in Orlando , Fla . , where Ibragim Todashev was killed by an F.B.I. agent last month .";
    	string sline = "An apartment complex in Orlando , Fla . ,";
//    	string aline = "21-27 ||| 6-0 4-1 3-2 1-5 2-6 5-7 8-7 10-8 10-9 11-10 11-11 18-13 19-14 16-15 16-16 16-17 17-18 14-19 13-20 12-21 13-21 12-22 9-23 9-24 9-25 20-26";
    	string aline = "9-8 ||| 6-0 4-1 3-2 1-5 2-6 5-7 8-7";
    	FeatureSet f;
    	f.ParseConfiguration("seq=X");
    	vector<ReordererRunner::OutputType> outputs;
    	outputs.push_back(ReordererRunner::OUTPUT_STRING);
    	outputs.push_back(ReordererRunner::OUTPUT_ORDER);
    	outputs.push_back(ReordererRunner::OUTPUT_ALIGN);
    	ConfigIParserGold config;
    	char * argv[] = {"", "-insert", "true",
    						"-delete" , "false",
    						"-attach_null", "right",
    						"-attach_trg", "left",
    						"-combine_blocks", "false"};
    	config.loadConfig(11, argv);
    	ostringstream oss, ess;
    	OutputCollector collector(&oss,&ess);
		IParserGoldTask gold(0, sline, aline, &f, &outputs, config, &collector);
		gold.Run();
		tokenizer<char_separator<char> > outs(oss.str(), char_separator<char>("\t"));
		tokenizer<char_separator<char> >::iterator it = outs.begin();
		int ret = 1;
		if (*it != "Fla Orlando in <> An apartment complex , . ,"){
			cerr << "incorrect string: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "6 4 3 -1 0 1 2 5 7 8"){
			cerr << "incorrect order: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "0-0 1-1 2-2 3-3 3-4 5-5 6-6 7-7 9-7 "){
			cerr << "incorrect align: " << *it << endl;
			ret = 0;
		}
		return ret;
    }

    int TestGoldOutput377(){
//    	string sline = "The couple met in the early 2000s , when both were students at the University of St. Andrews in Scotland , and their relationship , which was later hailed as a fairy tale union , proceeded sporadically for several years until their wedding in April 2011 .";
    	string sline = "The couple met in the early 2000s , when both were students at the University of St. Andrews in Scotland , and";
//    	string aline = "47-53 ||| 1-0 6-2 6-3 6-4 5-5 7-6 19-7 16-8 17-9 14-10 12-11 9-12 11-13 11-14 8-15 20-16 2-17 21-18 20-19 27-20 27-21 33-22 31-23 32-23 31-24 32-24 29-25 28-26 26-27 28-27 34-28 22-29 22-30 22-31 23-32 45-34 45-35 44-36 44-37 41-38 41-39 41-40 42-41 40-42 40-43 38-44 39-45 37-46 36-47 36-48 36-49 35-50 35-51 46-52";
    	string aline = "22-20 ||| 1-0 6-2 6-3 6-4 5-5 7-6 19-7 16-8 17-9 14-10 12-11 9-12 11-13 11-14 8-15 20-16 2-17 21-18 20-19";
    	FeatureSet f;
    	f.ParseConfiguration("seq=X");
    	vector<ReordererRunner::OutputType> outputs;
    	outputs.push_back(ReordererRunner::OUTPUT_STRING);
    	outputs.push_back(ReordererRunner::OUTPUT_ORDER);
    	outputs.push_back(ReordererRunner::OUTPUT_ALIGN);
    	ConfigIParserGold config;
    	char * argv[] = {"", "-insert", "true",
    						"-delete" , "false",
    						"-attach_null", "right",
    						"-attach_trg", "left",
    						"-combine_blocks", "true"};
    	config.loadConfig(11, argv);
    	ostringstream oss, ess;
    	OutputCollector collector(&oss,&ess);
		IParserGoldTask gold(0, sline, aline, &f, &outputs, config, &collector);
		gold.Run();
		tokenizer<char_separator<char> > outs(oss.str(), char_separator<char>("\t"));
		tokenizer<char_separator<char> >::iterator it = outs.begin();
		int ret = 1;
		if (*it != "The couple <> 2000s in the early , in Scotland of St. Andrews the University at both were students when , met and"){
			cerr << "incorrect string: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "0 1 -1 6 3 4 5 7 18 19 15 16 17 13 14 12 9 10 11 8 20 2 21"){
			cerr << "incorrect order: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "1-0 2-1 3-2 3-3 3-4 6-5 7-6 9-7 11-8 12-9 14-10 15-11 16-12 18-13 19-15 20-16 21-17 22-18 20-19 "){
			cerr << "incorrect align: " << *it << endl;
			ret = 0;
		}
		return ret;
    }

    int TestGoldOutput317(){
//    	string sline = "No one suggested a breakthrough was imminent , though Sylvia Mathews Burwell , the new White House budget director , said the rift within the Republican Party could be an opening for compromise .";
    	string sline = "the budget director , said the rift within the Republican Party could be an opening for compromise .";
//    	string aline = "4-0 6-2 6-3 6-4 6-5 0-7 1-7 0-8 1-8 2-9 2-10 2-11 8-12 8-13 8-14 7-15 9-16 10-17 11-18 12-19 14-20 15-21 16-21 17-22 18-23 19-25 25-26 26-26 23-27 25-27 26-27 23-28 22-31 32-33 31-34 30-35 28-37 27-38 28-38 27-39 27-40 20-41 20-42 20-43 20-44 33-45";
    	string aline = "18-24 ||| 1-0 2-1 3-3 9-4 10-4 7-5 9-5 10-5 7-6 6-9 16-11 15-12 14-13 12-15 11-16 12-16 11-17 11-18 4-19 4-20 4-21 4-22 17-23";
    	FeatureSet f;
    	f.ParseConfiguration("seq=X");
    	vector<ReordererRunner::OutputType> outputs;
    	outputs.push_back(ReordererRunner::OUTPUT_STRING);
    	outputs.push_back(ReordererRunner::OUTPUT_ORDER);
    	outputs.push_back(ReordererRunner::OUTPUT_ALIGN);
    	ConfigIParserGold config;
    	char * argv[] = {"", "-insert", "true",
    						"-delete" , "false",
    						"-attach_null", "right",
    						"-attach_trg", "left",
    						"-combine_blocks", "true"};
    	config.loadConfig(11, argv);
    	ostringstream oss, ess;
    	OutputCollector collector(&oss,&ess);
		IParserGoldTask gold(0, sline, aline, &f, &outputs, config, &collector);
		gold.Run();
		tokenizer<char_separator<char> > outs(oss.str(), char_separator<char>("\t"));
		tokenizer<char_separator<char> >::iterator it = outs.begin();
		int ret = 1;
		if (*it != "the budget director <> , within the Republican Party <> the rift <> compromise for an opening <> could be said ."){
			cerr << "incorrect string: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "0 1 2 -1 3 7 8 9 10 -1 5 6 -1 16 15 13 14 -1 11 12 4 17"){
			cerr << "incorrect order: " << *it << endl;
			ret = 0;
		}
		it++;
		if (*it != "1-0 2-1 3-2 4-3 5-5 5-6 7-4 7-5 8-4 8-5 9-7 9-8 11-9 12-10 13-11 14-12 16-13 17-14 18-16 18-17 18-18 19-15 19-16 20-19 20-20 20-21 20-22 21-23 "){
			cerr << "incorrect align: " << *it << endl;
			ret = 0;
		}
		return ret;
    }

    bool RunTest() {
    	int done = 0, succeeded = 0;
    	done++; cout << "TestInertRank()" << endl; if(TestInsertRank()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestGetReference()" << endl; if(TestGetReference()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestMixed()" << endl; if(TestMixed()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestInverted()" << endl; if(TestInverted()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestNullSourceMultiple()" << endl; if(TestNullSourceMultiple()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestNullTargetFront()" << endl; if(TestNullTargetFront()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestNullMixed()" << endl; if(TestNullMixed()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestNullLeftRight()" << endl; if(TestNullLeftRight()) succeeded++; else cout << "FAILED!!!" << endl;
    	done++; cout << "TestDiscontiuousTarget()" << endl; if(TestDiscontinuousTarget()) succeeded++; else cout << "FAILED!!!" << endl;
//    	done++; cout << "TestDiscontiuousTargetWithNull()" << endl; if(TestDiscontinuousTargetWithNull()) succeeded++; else cout << "FAILED!!!" << endl;
//    	done++; cout << "Test122()" << endl; if(Test122()) succeeded++; else cout << "FAILED!!!" << endl;
//    	done++; cout << "TestGoldOutput()" << endl; if(TestGoldOutput()) succeeded++; else cout << "FAILED!!!" << endl;
//		done++; cout << "TestGoldOutput377()" << endl; if(TestGoldOutput377()) succeeded++; else cout << "FAILED!!!" << endl;
//		done++; cout << "TestGoldOutput317()" << endl; if(TestGoldOutput317()) succeeded++; else cout << "FAILED!!!" << endl;
    	cout << "#### TestIParser Finished with "<<succeeded<<"/"<<done<<" tests succeeding ####"<<endl;
    	return done == succeeded;
    }
private:
    Ranks cal;
    FeatureDataSequence sent;
};
}


#endif /* TEST_IPARSER_H_ */
