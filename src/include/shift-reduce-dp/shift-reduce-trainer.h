/*
 * shift-reduce-trainer.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef PERCEPTRON_H_
#define PERCEPTRON_H_

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
using namespace std;

namespace lader {

class ShiftReduceTrainer {
	// A task
	class ShiftReduceTask : public Task {
	public:
		ShiftReduceTask(int id, const Sentence & data, const Ranks & ranks,
				ReordererModel * model, FeatureSet * features, const ConfigTrainer& config,
				Parser::Result & result, OutputCollector & collector, vector<Parser::Result> & kbest) :
					id_(id), data_(data), ranks_(ranks), model_(model), features_(features), config_(config),
					collector_(collector), result_(result), kbest_(kbest) { }
		void Run(){
			int verbose = config_.GetInt("verbose");
			int sent = id_;
			ostringstream err;
			if (verbose >= 1){
				cerr << endl << "Sentence " << sent << endl;
				cerr << "Rank: ";
				BOOST_FOREACH(int rank, ranks_.GetRanks())
					cerr << rank << " ";
				cerr << endl;
			}
			vector<DPState::Action> refseq = ranks_.GetReference();
			if (verbose >= 1){
				cerr << "Reference: ";
				BOOST_FOREACH(DPState::Action action, refseq)
					cerr << action << " ";
				cerr << endl;
			}
			Parser p;
			p.SetBeamSize(config_.GetInt("beam"));
			p.SetVerbose(config_.GetInt("verbose"));
			p.Search(*model_, *features_, data_, result_, config_.GetInt("max_state"));
			p.GetKbestResult(kbest_);
			if (verbose >= 1){
				cerr << "Result:   ";
				for (int step = 0 ; step < (const int)result_.actions.size() ; step++)
					cerr << " " << result_.actions[step];
				cerr << endl;
				cerr << "Purmutation:";
				BOOST_FOREACH(int order, result_.order)
					cerr << " " << order;
				cerr << endl;
			}
			collector_.Write(id_, "", err.str());
		}
	protected:
		int id_;
		const Sentence & data_;
		const Ranks & ranks_;
		ReordererModel * model_; // The model
		FeatureSet * features_;  // The mapping on feature ids and which to use
		const ConfigTrainer& config_;
		Parser::Result & result_;
		OutputCollector & collector_;
		vector<Parser::Result> & kbest_;
	};
public:
	ShiftReduceTrainer();
	virtual ~ShiftReduceTrainer();

	// Initialize the model
	void InitializeModel(const ConfigTrainer & config);

	// Read in the data
	void ReadData(const std::string & source_in, std::vector<Sentence*> & datas);

	// Read in the alignments
	void ReadAlignments(const std::string & align_in,
			std::vector<Ranks*> & ranks, std::vector<Sentence*> & datas);

	// Read in the parses
	void ReadParses(const std::string & align_in);
    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
    void TrainIncremental(const ConfigTrainer & config);

    // Write the model to a file
    void WriteModel(const std::string & str) {
        std::ofstream out(str.c_str());
        if(!out) THROW_ERROR("Couldn't write model to file " << str);
        features_->ToStream(out);
        model_->ToStream(out);
    }

private:
	double learning_rate_;
	std::vector<Ranks*> train_ranks_; // The alignments to use in training
	std::vector<Ranks*> dev_ranks_; // The alignments to use in development
	std::vector<FeatureDataParse> parses_; // The parses to use in training
	std::vector<LossBase*> losses_; // The loss functions
	std::vector<Sentence*> train_data_; // The training data
	std::vector<Sentence*> dev_data_; // The development data
	FeatureSet* features_;  // The mapping on feature ids and which to use
	ReordererModel* model_; // The model
	CombinedAlign::NullHandler attach_; // Where to attach nulls
	CombinedAlign::BlockHandler combine_; // Whether to combine blocks
	CombinedAlign::BracketHandler bracket_; // Whether to handle brackets

};

} /* namespace lader */
#endif /* PERCEPTRON_H_ */
