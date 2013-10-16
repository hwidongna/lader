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
                         bracket_(CombinedAlign::ALIGN_BRACKET_SPANS),
                         bilingual_features_(NULL) { }
    ~ReordererTrainer() {
        BOOST_FOREACH(std::vector<FeatureDataBase*> vec, data_)
            BOOST_FOREACH(FeatureDataBase* ptr, vec)
                delete ptr;
        BOOST_FOREACH(LossBase * loss, losses_)
            delete loss;
        BOOST_FOREACH(HyperGraph * graph, saved_graphs_)
        	if (graph)
        		delete graph;
        delete model_;
        delete features_;
        if (bilingual_features_)
        	delete bilingual_features_;
    }

    // Initialize the model
    void InitializeModel(const ConfigTrainer & config);

    // Read in the reference data
    void ReadTargetData(const std::string & ref_in);
    // Read in the data
    void ReadData(const std::string & source_in);

    // Read in the alignments
    void ReadAlignments(const std::string & align_in);

    // Read in the alignments
    void ReadSrc2TrgAlignments(const std::string & align_in);

    // Read in the parses
    void ReadParses(const std::string & align_in);

    // Write the model to a file
    void WriteModel(const std::string & str);

    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
    void TrainIncremental(const ConfigTrainer & config);

private:

    std::vector<Ranks> ranks_; // The alignments to use in training
    std::vector<FeatureDataParse> parses_; // The parses to use in training
    std::vector<Sentence> data_; // The data
    ReordererModel * model_; // The model
    FeatureSet * features_;  // The mapping on feature ids and which to use
    std::vector<LossBase*> losses_; // The loss functions
    double learning_rate_; // The learning rate
    std::vector<HyperGraph*> saved_graphs_; // Features for each hypergraph
    CombinedAlign::NullHandler attach_; // Where to attach nulls
    CombinedAlign::BlockHandler combine_; // Whether to combine blocks
    CombinedAlign::BracketHandler bracket_; // Whether to handle brackets

    // they are optional
    FeatureSet * bilingual_features_;  // The mapping on feature ids and which to use
    std::vector<Sentence> trg_data_; // The data for the bilingual features
    std::vector<CombinedAlign> align_; // The alignments to use in training
};

}

#endif

