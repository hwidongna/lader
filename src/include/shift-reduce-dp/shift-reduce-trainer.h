/*
 * shift-reduce-trainer.h
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#ifndef PERCEPTRON_H_
#define PERCEPTRON_H_

#include <lader/feature-vector.h>
#include <lader/config-trainer.h>
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
using namespace std;

namespace lader {

class ShiftReduceTrainer {
public:
	ShiftReduceTrainer();
	virtual ~ShiftReduceTrainer();
	struct Result{
		double similarity;
		FeatureVectorInt deltafeats;
		int steps;
	};

	// Initialize the model
	void InitializeModel(const ConfigTrainer & config);

	// Read in the data
	void ReadData(const std::string & source_in, std::vector<Sentence> & datas);

	// Read in the alignments
	void ReadAlignments(const std::string & align_in,
			std::vector<Ranks> & ranks, std::vector<Sentence> & datas);

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
	std::vector<Ranks> train_ranks_; // The alignments to use in training
	std::vector<Ranks> dev_ranks_; // The alignments to use in development
	std::vector<FeatureDataParse> parses_; // The parses to use in training
	std::vector<LossBase*> losses_; // The loss functions
	std::vector<Sentence> train_data_; // The training data
	std::vector<Sentence> dev_data_; // The development data
	FeatureSet* features_;  // The mapping on feature ids and which to use
	ReordererModel* model_; // The model
	CombinedAlign::NullHandler attach_; // Where to attach nulls
	CombinedAlign::BlockHandler combine_; // Whether to combine blocks
	CombinedAlign::BracketHandler bracket_; // Whether to handle brackets

};

} /* namespace lader */
#endif /* PERCEPTRON_H_ */
