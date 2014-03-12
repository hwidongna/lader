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
#include <shift-reduce-dp/ddpstate.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>
#include <shift-reduce-dp/config-gold.h>
#include <boost/tokenizer.hpp>
#include <shift-reduce-dp/shift-reduce-runner.h>
using namespace std;
using namespace boost;

namespace lader {

// A task
class ShiftReduceGoldTask : public ShiftReduceTask {
public:
	ShiftReduceGoldTask(int id, const Sentence * data, const Ranks * ranks,
			vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				ShiftReduceTask(id, "", NULL, NULL, outputs, config, collector),
				data_(data), ranks_(ranks)	{ }
    void Run()
    {
        int sent = id_;
        int verbose = config_.GetInt("verbose");
        if(verbose >= 1){
            ess << endl << "Sentence " << sent << endl;
			ess << "Rank:";
			BOOST_FOREACH(int rank, ranks_->GetRanks())
				ess << " " << rank;
			ess << endl;
		}
		ActionVector refseq = ranks_->GetReference();
		if (verbose >= 1){
			ess << "Reference:";
			BOOST_FOREACH(DPState::Action action, refseq)
				ess << " " << (char) action;
			ess << endl;
		}
		const Sentence & datas = (*data_);
		int n = datas[0]->GetNumWords();
		if (refseq.size() < 2*n - 1)
			THROW_ERROR("Fail to get correct reference sequence" << endl)
		int max_step = refseq.size() + 1;
		DPStateVector stateseq;
		DPState * state = new DDPState();
		stateseq.push_back(state);
		for (int step = 1 ; step < max_step ; step++){
			DPState::Action action = refseq[step-1];
			if (state->Allow(action, n))
				state->Take(action, stateseq, true);
			state = stateseq.back(); // only one item
		}
		Output(datas, state);
		collector_->Write(id_, oss.str(), ess.str());
		BOOST_FOREACH(DPState * state, stateseq)
			delete state;
	}
protected:
	const Sentence * data_;
	const Ranks * ranks_;
};

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
		ReadAlignments(config.GetString("align_in"), ranks_, data_);
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
		OutputCollector collector;
    	ThreadPool pool(config.GetInt("threads"), 1000);
    	for (int sent = 0 ; sent < data_.size() ; sent++) {
    		Task *task = new ShiftReduceGoldTask(sent, data_[sent],
					ranks_[sent], &outputs, config, &collector);
    		pool.Submit(task);
    	}
    	pool.Stop(true);
	}
};

}
#endif /* GOLD_STANDARD_H_ */
