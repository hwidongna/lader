/*
 * iparser-evaluator.cc
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#include <shift-reduce-dp/iparser-evaluator.h>
#include <shift-reduce-dp/iparser-ranks.h>
#include <shift-reduce-dp/iparser.h>
#include <shift-reduce-dp/flat-tree.h>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <lader/feature-data-sequence.h>
#include <lader/loss-base.h>
#include <iostream>
#include <fstream>

using namespace std;
using namespace boost;

namespace lader{

void IParserEvaluator::InitializeModel(const ConfigBase & config){
	ReordererEvaluator::InitializeModel(config);
    attach_trg_ = config.GetString("attach_trg") == "left" ?
                CombinedAlign::ATTACH_NULL_LEFT :
                CombinedAlign::ATTACH_NULL_RIGHT;
}

// Run the evaluator
void IParserEvaluator::Evaluate(const ConfigBase & config){
	InitializeModel(config);
	// Set up the losses
	vector<LossBase*> losses_;
	vector<string> losses, first_last;
	algorithm::split(
			losses, config.GetString("loss_profile"), is_any_of("|"));
	BOOST_FOREACH(string s, losses) {
		algorithm::split(first_last, s, is_any_of("="));
		if(first_last.size() != 2) THROW_ERROR("Bad loss: " << s);
		LossBase * loss = LossBase::CreateNew(first_last[0]);
		double dub;
		istringstream iss(first_last[1]);
		iss >> dub;
		loss->SetWeight(dub);
		losses_.push_back(loss);
	}
	int verbose = config.GetInt("verbose");
	string placeholder = config.GetString("placeholder");
	// Calculate the losses
	vector<pair<double,double> > sums(losses_.size(), pair<double,double>(0,0));

	// Open the files
	const vector<string> & args = config.GetMainArgs();
	ifstream aligns_in(SafeAccess(args, 0).c_str());
	if(!aligns_in) THROW_ERROR("Couldn't find alignment file " << args[0]);
	ifstream data_in(SafeAccess(args, 1).c_str());
	if(!data_in) THROW_ERROR("Couldn't find input file " << args[1]);
	ifstream *src_in = NULL, *trg_in = NULL;
	if(args.size() > 2) {
		src_in = new ifstream(SafeAccess(args, 2).c_str());
		if(!(*src_in)) THROW_ERROR("Couldn't find source file " << args[2]);
	}
	if(args.size() > 3) {
		trg_in = new ifstream(SafeAccess(args, 3).c_str());
		if(!*trg_in) THROW_ERROR("Couldn't find target file " << args[3]);
	}
	cerr << "Sentence Length" << endl;
	string data, align, src, trg;
	// Read them one-by-one and run the evaluator
	for(int sent = 0 ; getline(data_in, data) && getline(aligns_in, align) ; sent++) {
		cout << "Sentence " << sent << endl;
		// Get the input values
		std::vector<string> datas;
		algorithm::split(datas, data, is_any_of("\t"));
		istringstream iss(datas[0]);
		std::vector<int> order; string ival;
		try {
			while(iss >> ival) order.push_back(lexical_cast<int>(ival));
		} catch(std::exception e) {
			THROW_ERROR("Expecting a string of integers in the lader reordering file, but got" << endl << datas[0]);
		}
		// Get the source file
		getline(*src_in, src);
		vector<string> srcs;
		algorithm::split(srcs, src, is_any_of(" "));
		FeatureDataSequence words;
		words.FromString(src);
		// Print the input values
		cout << "sys_ord:\t" << datas[0] << endl;
		for(int i = 1; i < (int)datas.size(); i++)
			cout << "sys_"<<i<<":\t" << datas[i] << endl;
		// If source and target files exist, print them as well
		cout << "src:\t" << src << endl;
		if(trg_in) {
			getline(*trg_in, trg);
			cout << "trg:\t" << trg << endl;
		}
		Alignment al = Alignment::FromString(align);
        IParserRanks ranks(CombinedAlign(srcs, al, attach_, combine_, bracket_), attach_trg_);
        CombinedAlign cal(srcs, al, CombinedAlign::LEAVE_NULL_AS_IS, combine_, bracket_);
        ranks.Insert(&cal); // TODO: optional?
		ActionVector refseq = ranks.GetReference(&cal);
		if (!refseq.empty()){
			cout << "ref:\t";
			BOOST_FOREACH(DPState::Action action, refseq)
				cout << " " << (char)action;
			cout << endl;
			int n = (refseq.size()+1) / 2;
			IParser gparser(n, n);
			DPState * goal = gparser.GuidedSearch(refseq, n);
			if (goal == NULL){
				if (verbose >= 1)
					cerr << "Parser cannot produce the merged reference sequence, skip it" << endl;
				continue;
			}
			Parser::Result gresult;
			Parser::SetResult(gresult, goal);
			cout << "ref:\t";
			BOOST_FOREACH(int order, gresult.order)
				cout << " " << order;
			cout << endl;
			// Print the reference reordering
			cout << "ref:\t";
			for(int i = 0; i < (int)gresult.order.size(); i++) {
				if(i != 0) cout << " ";
				if(gresult.order[i] < 0)
					cout << placeholder;
				else
					cout << srcs[gresult.order[i]];
			}
			cout << endl;
			// Print the reference tree
			cout << "ref:\t";
			IDPStateNode node(0, words.GetNumWords(), NULL, DPState::INIT);
			node.AddChild(node.Flatten(goal));
			node.PrintParse(words.GetSequence(), cout);
			cout << endl;
			// Get the ranks
			Ranks ranks;
			ranks.SetRanks(gresult.order);
			cerr << sent << "\t" << words.GetNumWords();
			// Score the values
			for(int i = 0; i < (int) losses.size(); i++) {
				pair<double,double> my_loss =
						losses_[i]->CalculateSentenceLoss(order,&ranks,NULL);
				sums[i].first += my_loss.first;
				sums[i].second += my_loss.second;
				double acc = my_loss.second == 0 ?
						1 : (1-my_loss.first/my_loss.second);
				if(i != 0) cout << "\t";
				cout << losses_[i]->GetName() << "=" << acc
						<< " (loss "<<my_loss.first<< "/" <<my_loss.second<<")";
				cerr << "\t" << acc;
			}
			cerr << endl;
		}
		cout << endl << endl;
	}
	cout << "Total:" << endl;
	for(int i = 0; i < (int) sums.size(); i++) {
		if(i != 0) cout << "\t";
		cout << losses_[i]->GetName() << "="
                		 << (1 - sums[i].first/sums[i].second)
                		 << " (loss "<<sums[i].first << "/" <<sums[i].second<<")";
	}
	cout << endl;
}

}
