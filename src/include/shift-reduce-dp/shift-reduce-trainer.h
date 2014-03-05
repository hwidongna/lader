/*
 * shift-reduce-trainer.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef SHIFT_REDUCE_TRAINER_H_
#define SHIFT_REDUCE_TRAINER_H_

#include <lader/feature-vector.h>
#include <shift-reduce-dp/config-trainer.h>
#include <fstream>
#include <tr1/unordered_map>
#include <lader/util.h>
#include <lader/feature-data-base.h>
#include <lader/reorderer-model.h>
#include <iostream>
#include <lader/combined-alignment.h>
#include <lader/feature-data-parse.h>
#include <lader/ranks.h>
#include <lader/loss-base.h>
#include <lader/thread-pool.h>
#include <lader/output-collector.h>
#include <shift-reduce-dp/parser.h>
#include <lader/reorderer-trainer.h>
using namespace std;

namespace lader {

class ShiftReduceTrainer : public ReordererTrainer {
	// A task
	class ShiftReduceTask : public Task {
	public:
		ShiftReduceTask(int id, const Sentence & data, const Ranks & ranks,
				ReordererModel * model, FeatureSet * features, const ConfigBase& config,
				Parser::Result & result, OutputCollector & collector, vector<Parser::Result> & kbest) :
					id_(id), data_(data), ranks_(ranks), model_(model), features_(features), config_(config),
					collector_(collector), result_(result), kbest_(kbest) { }
		void Run(){
			int verbose = config_.GetInt("verbose");
			int sent = id_;
			ostringstream err;
			if (verbose >= 1){
				err << endl << "Sentence " << sent << endl;
				err << "Rank: ";
				BOOST_FOREACH(int rank, ranks_.GetRanks())
					err << rank << " ";
				err << endl;
			}
			vector<DPState::Action> refseq = ranks_.GetReference();
			if (verbose >= 1){
				err << "Reference: ";
				BOOST_FOREACH(DPState::Action action, refseq)
					err << action << " ";
				err << endl;
			}
			Parser p;
			p.SetBeamSize(config_.GetInt("beam"));
			p.SetVerbose(config_.GetInt("verbose"));
			p.Search(*model_, *features_, data_, result_, config_.GetInt("max_state"));
			p.GetKbestResult(kbest_);
			if (verbose >= 1){
				err << "Result:   ";
				for (int step = 0 ; step < (const int)result_.actions.size() ; step++)
					err << " " << result_.actions[step];
				err << endl;
				err << "Purmutation:";
				BOOST_FOREACH(int order, result_.order)
					err << " " << order;
				err << endl;
			}
			collector_.Write(id_, "", err.str());
		}
	protected:
		int id_;
		const Sentence & data_;
		const Ranks & ranks_;
		ReordererModel * model_; // The model
		FeatureSet * features_;  // The mapping on feature ids and which to use
		const ConfigBase& config_;
		Parser::Result & result_;
		OutputCollector & collector_;
		vector<Parser::Result> & kbest_;
	};
public:
	ShiftReduceTrainer() { }
	virtual ~ShiftReduceTrainer() {
	    BOOST_FOREACH(Sentence * vec, dev_data_)
			if (vec){
				BOOST_FOREACH(FeatureDataBase* ptr, *vec)
					delete ptr;
				delete vec;
			}
	    BOOST_FOREACH(Ranks * ranks, dev_ranks_)
			if (ranks)
				delete ranks;
	}

    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
    void TrainIncremental(const ConfigBase & config);

private:
	std::vector<Ranks*> dev_ranks_; // The alignments to use in development
	std::vector<Sentence*> dev_data_; // The development data

};

} /* namespace lader */
#endif /* SHIFT_REDUCE_TRAINER_H_ */
