#ifndef REORDERER_TRAINER_H__ 
#define REORDERER_TRAINER_H__

#include <fstream>
#include <lader/config-trainer.h>
#include <lader/reorderer-model.h>
#include <lader/feature-set.h>
#include <lader/loss-base.h>

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
        BOOST_FOREACH(EdgeFeatureMap * feat, saved_feats_) {
            BOOST_FOREACH(EdgeFeaturePair feat_pair, *feat){
            	delete feat_pair.first;
                delete feat_pair.second;
            }
            delete feat;
        }
        delete model_;
        delete features_;
        delete non_local_features_;
    }

    // Initialize the model
    void InitializeModel(const ConfigTrainer & config);

    // Read in the data
    void ReadData(const std::string & source_in) {
        std::ifstream in(source_in.c_str());
        if(!in) THROW_ERROR("Could not open source file (-source_in): "
                                <<source_in);
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
    void WriteModel(const std::string & str) {
        std::ofstream out(str.c_str());
        if(!out) THROW_ERROR("Couldn't write model to file " << str);
        features_->ToStream(out);
        non_local_features_->ToStream(out);
        model_->ToStream(out);
    }

    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
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
    std::vector<EdgeFeatureMap*> saved_feats_; // Features for each hypergraph
    CombinedAlign::NullHandler attach_; // Where to attach nulls
    CombinedAlign::BlockHandler combine_; // Whether to combine blocks
    CombinedAlign::BracketHandler bracket_; // Whether to handle brackets

};

}

#endif

