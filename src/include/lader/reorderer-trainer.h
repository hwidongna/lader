#ifndef REORDERER_TRAINER_H__ 
#define REORDERER_TRAINER_H__

#include <fstream>
#include <lader/config-trainer.h>
#include <lader/hyper-graph.h>

namespace lader {

// A class to train the reordering model
class ReordererTrainer {
public:

    ReordererTrainer() : learning_rate_(1),
                         attach_(CombinedAlign::ATTACH_NULL_LEFT),
                         combine_(CombinedAlign::COMBINE_BLOCKS),
                         bracket_(CombinedAlign::ALIGN_BRACKET_SPANS) { }
    virtual ~ReordererTrainer() {
	    BOOST_FOREACH(Sentence * vec, data_)
	    	if (vec){
	    		BOOST_FOREACH(FeatureDataBase* ptr, *vec)
	    			delete ptr;
	    		delete vec;
	    	}
	    BOOST_FOREACH(Ranks * ranks, ranks_)
	    	if (ranks)
	        	delete ranks;
        BOOST_FOREACH(LossBase * loss, losses_)
            delete loss;
        BOOST_FOREACH(HyperGraph * graph, saved_graphs_)
        	if (graph)
        		delete graph;
        if(model_) delete model_;
        if(features_) delete features_;
    }

    // Initialize the model
    virtual void InitializeModel(const ConfigBase & config);


	// Read in the data
	void ReadData(const std::string & source_in, std::vector<Sentence*> & datas);

	// Read in the alignments
	void ReadAlignments(const std::string & align_in,
			std::vector<Ranks*> & ranks, std::vector<Sentence*> & datas);
    // Read in the parses
    void ReadParses(const std::string & align_in);

    // Write the model to a file
    void WriteModel(const std::string & str) {
        std::ofstream out(str.c_str());
        if(!out) THROW_ERROR("Couldn't write model to file " << str);
        features_->ToStream(out);
        model_->ToStream(out);
    }

    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
    virtual void TrainIncremental(const ConfigBase & config);

protected:
    std::vector<Ranks*> ranks_; // The alignments to use in training
    std::vector<Sentence*> data_; // The data to use in training
    ReordererModel* model_; // The model
    FeatureSet* features_;  // The mapping on feature ids and which to use
    std::vector<LossBase*> losses_; // The loss functions
    CombinedAlign::NullHandler attach_; // Where to attach nulls
    CombinedAlign::BlockHandler combine_; // Whether to combine blocks
    CombinedAlign::BracketHandler bracket_; // Whether to handle brackets
private:

    std::vector<FeatureDataParse> parses_; // The parses to use in training
    double learning_rate_; // The learning rate
    std::vector<HyperGraph*> saved_graphs_; // Features for each hypergraph

};

}

#endif

