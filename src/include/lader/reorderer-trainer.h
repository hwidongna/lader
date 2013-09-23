#ifndef REORDERER_TRAINER_H__ 
#define REORDERER_TRAINER_H__

#include <fstream>
#include <lader/config-trainer.h>
#include <lader/reorderer-model.h>
#include <lader/feature-set.h>
#include <lader/loss-base.h>
#include <lader/hyper-graph.h>

namespace lader {

// A class to train the reordering model
class ReordererTrainer {
public:

    ReordererTrainer() : learning_rate_(1),
                         attach_(CombinedAlign::ATTACH_NULL_LEFT),
                         combine_(CombinedAlign::COMBINE_BLOCKS),
                         bracket_(CombinedAlign::ALIGN_BRACKET_SPANS) { }
    ~ReordererTrainer() {
        BOOST_FOREACH(std::vector<FeatureDataBase*> vec, data_)
            BOOST_FOREACH(FeatureDataBase* ptr, vec)
                delete ptr;
        BOOST_FOREACH(LossBase * loss, losses_)
            delete loss;
        BOOST_FOREACH(HyperGraph * graph, saved_graphs_)
        	delete graph;
        delete model_;
        delete features_;
        delete non_local_features_;
    }

    void InitializeModel(const ConfigTrainer & config);
    void ReadData(const std::string & source_in)
    {
        std::ifstream in(source_in.c_str());
        if(!in) THROW_ERROR("Could not open source file (-source_in): "
                                <<source_in)
        std::string line;
        data_.clear();
        while(getline(in, line))
            data_.push_back(features_->ParseInput(line));
    }
    // Read in the alignments
    void ReadAlignments(const std::string & align_in);

    // Read in the parses
    void ReadParses(const std::string & align_in);

    // Write the model to a file
    void WriteModel(const std::string & str, bool backward_compatibility) {
        std::ofstream out(str.c_str());
        if(!out) THROW_ERROR("Couldn't write model to file " << str)
        features_->ToStream(out);
        non_local_features_->ToStream(out);
        if (backward_compatibility)
        	model_->ToStreamOld(out);
        else
        	model_->ToStream(out);
    }

    void TrainIncremental(const ConfigTrainer & config);

private:
    std::vector<Ranks> ranks_; // The alignments to use in training
    std::vector<FeatureDataParse> parses_; // The parses to use in training
    std::vector<std::vector<FeatureDataBase*> > data_; // The data
    ReordererModel * model_; // The model
    FeatureSet * features_;  // The mapping on feature ids and which to use
    FeatureSet * non_local_features_;  // The mapping on feature ids and which to use
    std::vector<LossBase*> losses_; // The loss functions
    double learning_rate_; // The learning rate
    std::vector<HyperGraph*> saved_graphs_; // Features for each hypergraph
    CombinedAlign::NullHandler attach_; // Where to attach nulls
    CombinedAlign::BlockHandler combine_; // Whether to combine blocks
    CombinedAlign::BracketHandler bracket_; // Whether to handle brackets
};

}

#endif

