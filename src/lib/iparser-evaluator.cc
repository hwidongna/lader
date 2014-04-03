/*
 * iparser-evaluator.cc
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#include <shift-reduce-dp/iparser-evaluator.h>
#include <shift-reduce-dp/iparser.h>
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

void IParserEvaluator::ReadGold(const string & gold_in, vector<ActionVector> & golds) {
	std::ifstream in(gold_in.c_str());
	if(!in) THROW_ERROR("Could not open gold file: "
			<<gold_in);
	std::string line;
	golds.clear();
	while(getline(in, line)){
		vector<string> columns;
		algorithm::split(columns, line, is_any_of("\t"));
		golds.push_back(DPState::ActionFromString(columns[0]));
	}
}

// Run the evaluator
void IParserEvaluator::Evaluate(const ConfigBase & config){
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
	ReadGold(config.GetString("source_gold"), source_gold_);
	ReadGold(config.GetString("target_gold"), target_gold_);
	int verbose = config.GetInt("verbose");
	string placeholder = config.GetString("placeholder");
	// Calculate the losses
	vector<pair<double,double> > sums(losses_.size(), pair<double,double>(0,0));

	// Open the files
	const vector<string> & args = config.GetMainArgs();
	ifstream data_in(SafeAccess(args, 0).c_str());
	if(!data_in) THROW_ERROR("Couldn't find input file " << args[0]);
	ifstream *src_in = NULL, *trg_in = NULL;
	if(args.size() > 1) {
		src_in = new ifstream(SafeAccess(args, 1).c_str());
		if(!(*src_in)) THROW_ERROR("Couldn't find source file " << args[1]);
	}
	if(args.size() > 2) {
		trg_in = new ifstream(SafeAccess(args, 2).c_str());
		if(!*trg_in) THROW_ERROR("Couldn't find target file " << args[2]);
	}
	cerr << "Sentence Length Edit-distance" << endl;
	string data, align, src, trg;
	// Read them one-by-one and run the evaluator
	for(int sent = 0 ; getline(data_in, data) ; sent++) {
		ActionVector & frefseq = source_gold_[sent];
		ActionVector & erefseq = target_gold_[sent];
		if (verbose >= 1){
			cerr << endl << "Sentence " << sent << endl;
			cerr << "Source Reference:";
			BOOST_FOREACH(DPState::Action action, frefseq)
			cerr << " " << (char)action;
			cerr << endl;
			cerr << "Target Reference:";
			BOOST_FOREACH(DPState::Action action, erefseq)
			cerr << " " << (char)action;
			cerr << endl;
		}
		if (frefseq.empty() || erefseq.empty()){
			if (verbose >= 1)
				cerr << "Parser cannot produce the reference sequence, skip it" << endl;
			continue;
		}
		int J = (frefseq.size()+1) / 2;
		IParser fparser(J, J);
		DPState * fgoal = fparser.GuidedSearch(frefseq, J);
		if (fgoal == NULL){
			if (verbose >= 1)
				cerr << "Parser cannot produce the source reference sequence, skip it" << endl;
			continue;
		}
		int I = (erefseq.size()+1) / 2;
		IParser eparser(I, I);
		DPState * egoal = eparser.GuidedSearch(erefseq, I);
		if (egoal == NULL){
			if (verbose >= 1)
				cerr << "Parser cannot produce the target reference sequence, skip it" << endl;
			continue;
		}
		ActionVector refseq;
		IDPState::Merge(refseq, fgoal, egoal);
		if (verbose >= 1){
			cerr << "Merged Reference:";
			BOOST_FOREACH(DPState::Action action, refseq)
			cerr << " " << (char)action;
			cerr << endl;
		}
		int n = (refseq.size()+1) / 2;
		IParser gparser(n, n);
		DPState * goal = gparser.GuidedSearch(refseq, n);
		if (goal == NULL){
			if (verbose >= 1)
				cerr << "Parser cannot produce the reference sequence, skip it" << endl;
			continue;
		}
		Parser::Result gresult;
		Parser::SetResult(&gresult, goal);
		if (verbose >= 1){
			cerr << "Oracle Purmutation:";
			BOOST_FOREACH(int order, gresult.order)
				cerr << " " << order;
			cerr << endl;
		}
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
		// Get the ranks
		Ranks ranks;
		ranks.SetRanks(gresult.order);
		// Print the input values
		cout << "sys_ord:\t" << datas[0] << endl;
		for(int i = 1; i < (int)datas.size(); i++)
			cout << "sys_"<<i<<":\t" << datas[i] << endl;
		// If source and target files exist, print them as well
		cout << "src:\t" << src << endl;
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
		if(args.size() > 3) {
			getline(*trg_in, trg);
			cout << "trg:\t" << trg << endl;
		}
		FeatureDataSequence words;
		words.FromString(src);
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
