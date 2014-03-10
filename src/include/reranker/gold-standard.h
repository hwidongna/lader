/*
 * gold-standard.h
 *
 *  Created on: Mar 7, 2014
 *      Author: leona
 */

#ifndef GOLD_STANDARD_H_
#define GOLD_STANDARD_H_

#include <lader/reorderer-trainer.h>
#include <lader/reorderer-runner.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/parser.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>
#include <reranker/config-gold.h>
#include <boost/tokenizer.hpp>
using namespace std;
using namespace boost;

namespace reranker {


class GoldStandard : public ReordererTrainer{
public:
	GoldStandard() { }
	virtual ~GoldStandard() { }

	// Initialize the model
	void InitializeModel(const ConfigBase & config){
		features_ = new FeatureSet;
		features_->ParseConfiguration(config.GetString("feature_profile"));
		// Load the other config
		attach_ = config.GetString("attach_null") == "left" ?
				CombinedAlign::ATTACH_NULL_LEFT :
				CombinedAlign::ATTACH_NULL_RIGHT;
		combine_ = config.GetBool("combine_blocks") ?
				CombinedAlign::COMBINE_BLOCKS :
				CombinedAlign::LEAVE_BLOCKS_AS_IS;
		bracket_ = config.GetBool("combine_brackets") ?
				CombinedAlign::ALIGN_BRACKET_SPANS :
				CombinedAlign::LEAVE_BRACKETS_AS_IS;
	}

	void Run(const ConfigBase & config){
		InitializeModel(config);
		ReadData(config.GetString("source_in"), data_);
		if(config.GetString("align_in").length())
			ReadAlignments(config.GetString("align_in"), ranks_, data_);
		int verbose = config.GetInt("verbose");
		int m = config.GetInt("max_swap");
		vector<ReordererRunner::OutputType> outputs;
		tokenizer<char_separator<char> > outs(config.GetString("out_format"),
				char_separator<char>(","));
		BOOST_FOREACH(const string & str, outs) {
			if(str == "string")
				outputs.push_back(ReordererRunner::OUTPUT_STRING);
			else if(str == "parse")
				outputs.push_back(ReordererRunner::OUTPUT_PARSE);
			else if(str == "order")
				outputs.push_back(ReordererRunner::OUTPUT_ORDER);
			else if(str == "flatten")
				outputs.push_back(ReordererRunner::OUTPUT_FLATTEN);
			else if(str == "action")
				outputs.push_back(ReordererRunner::OUTPUT_ACTION);
			else
				THROW_ERROR("Bad output format '" << str <<"'");
		}
		cerr << "(\".\" == 100 sentences)" << endl;
		for (int sent = 0 ; sent < data_.size() ; sent++) {
			if (verbose >= 1){
				cerr << endl << "Sentence " << sent << endl;
				cerr << "Rank:";
				BOOST_FOREACH(int rank, ranks_[sent]->GetRanks())
					cerr << " " << rank;
				cerr << endl;
			}
			vector<DPState::Action> refseq = ranks_[sent]->GetReference(m);
			if (verbose >= 1){
				cerr << "Reference:";
				BOOST_FOREACH(DPState::Action action, refseq)
					cerr << " " << (char) action;
				cerr << endl;
			}
			Sentence & datas = (*data_[sent]);
			int n = datas[0]->GetNumWords();
			vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
			if (refseq.size() < 2*(n+m) - 1){
				if (verbose >= 1)
					cerr << "Fail to get correct reference sequence, skip it" << endl;
			} else{
				DPStateVector stateseq;
				if (m > 0)
					stateseq.push_back(new DDPState());
				else
					stateseq.push_back(new DPState());
				BOOST_FOREACH(DPState::Action action, refseq){
					DPState * state = stateseq.back();
					state->Take(action, stateseq, true);
				}
				// for a complete tree
				DPState * goal = stateseq.back();
				// Reorder
				Parser::Result result;
				SetResult(&result, goal);
				std::vector<int> & reordering = result.order;
				datas[0]->Reorder(reordering);
				std::vector<DPState::Action> & actions = result.actions;
				for(int i = 0; i < (int)outputs.size(); i++) {
					if(i != 0) cout << "\t";
					if(outputs.at(i) == ReordererRunner::OUTPUT_STRING) {
						cout << datas[0]->ToString();
					} else if(outputs.at(i) == ReordererRunner::OUTPUT_PARSE) {
						goal->PrintParse(words, cout);
					} else if(outputs.at(i) == ReordererRunner::OUTPUT_ORDER) {
						for(int j = 0; j < (int)reordering.size(); j++) {
							if(j != 0) cout << " ";
							cout << reordering[j];
						}
					} else if(outputs.at(i) == ReordererRunner::OUTPUT_FLATTEN) {
						DDPStateNode root(0, n, NULL, DPState::INIT);
						root.AddChild(root.Flatten(goal));
						root.PrintParse(words, cout);
					} else if(outputs.at(i) == ReordererRunner::OUTPUT_ACTION) {
						for(int j = 0; j < (int)actions.size(); j++) {
							if(j != 0) cout << " ";
							cout << (char) actions[j];
						}
					} else {
						THROW_ERROR("Unimplemented output format");
					}
				}
				BOOST_FOREACH(DPState * state, stateseq)
					delete state;
			}
			cout << endl;
		}
	}
};

}
#endif /* GOLD_STANDARD_H_ */
