/*
 * gold-tree.h
 *
 *  Created on: Feb 26, 2014
 *      Author: leona
 */

#ifndef GOLD_TREE_H_
#define GOLD_TREE_H_

#include <lader/reorderer-trainer.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/parser.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>
#include <reranker/config-gold.h>

using namespace std;
namespace reranker {


class GoldTree : public ReordererTrainer{
public:
	GoldTree() { }
	virtual ~GoldTree() { }

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

		cerr << "(\".\" == 100 sentences)" << endl;
		for (int sent = 0 ; sent < data_.size() ; sent++) {
			if (verbose >= 1){
				cerr << endl << "Sentence " << sent << endl;
				cerr << "Rank:";
				BOOST_FOREACH(int rank, ranks_[sent]->GetRanks())
				cerr << " " << rank;
				cerr << endl;
			}
			vector<DPState::Action> refseq = ranks_[sent]->GetReference();
			if (verbose >= 1){
				cerr << "Reference:";
				BOOST_FOREACH(DPState::Action action, refseq)
				cerr << " " << action;
				cerr << endl;
			}
			int n = (*data_[sent])[0]->GetNumWords();
			if (refseq.size() < 2*n - 1){
				if (verbose >= 1)
					cerr << "Fail to get correct reference sequence, skip it" << endl;
			} else{
				DPStateVector stateseq;
				stateseq.push_back(new DPState());
				for (int step = 1 ; step < 2*n ; step++){
					DPState * state = stateseq.back();
					state->Take(refseq[step-1], stateseq, true);
				}
				// for a complete tree
				DPState * goal = stateseq.back();
				DPStateNode root(0, n, NULL, DPState::INIT);
				root.AddChild(root.Flatten(goal));
				root.PrintParse((*data_[sent])[0]->GetSequence(), cout);
			}
			cout << endl;
		}
	}
};


} /* namespace reranker */
#endif /* GOLD_TREE_H_ */
