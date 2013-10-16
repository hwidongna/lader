#ifndef REORDERER_RUNNER_H__ 
#define REORDERER_RUNNER_H__

#include <iostream>
#include <fstream>
#include <lader/config-runner.h>
#include <lader/reorderer-model.h>
#include <lader/feature-set.h>
#include <lader/loss-base.h>
#include <lader/thread-pool.h>
#include <lader/output-collector.h>
#include <lader/hyper-graph.h>

namespace lader {

// A class to train the reordering model
class ReordererRunner {
public:

    // Types of output to produce
    typedef enum {
        OUTPUT_STRING,
        OUTPUT_PARSE,
        OUTPUT_HYPERGRAPH,
        OUTPUT_ORDER
    } OutputType;

    ReordererRunner() { }
    ~ReordererRunner() {
        if(model_) delete model_;
        if(features_) delete features_;
    }

    // Initialize the model
    void InitializeModel(const ConfigRunner & config);
    
    // Run the model
    void Run(const ConfigRunner & config);

private:

    ReordererModel * model_; // The model
    FeatureSet * features_;  // The mapping on feature ids and which to use
    std::vector<OutputType> outputs_;

    // they are optional
	FeatureSet * bilingual_features_;  // The mapping on feature ids and which to use
	std::vector<Sentence> trg_data_; // The data for the bilingual features
	std::vector<CombinedAlign> align_; // The alignments to use in training
};

// A task
class ReordererTask : public Task {
public:
    ReordererTask(int id, const std::string & sline,
                  ReordererModel * model, FeatureSet * features,
                  std::vector<ReordererRunner::OutputType> * outputs,
                  const ConfigRunner& config, HyperGraph * hyper_graph,
                  OutputCollector * collector) :
        id_(id), sline_(sline), model_(model), features_(features),
        outputs_(outputs), collector_(collector), config_(config), graph_(hyper_graph) { }
    void SetBilingual(FeatureSet * bilingual_features,
    		std::string tline, std::string aline){
    	bilingual_features_ = bilingual_features;
    	tline_ = tline;
    	aline_ = aline;
    }
    void Run();
protected:
    int id_;
    std::string sline_;
    ReordererModel * model_; // The model
    FeatureSet * features_;  // The mapping on feature ids and which to use
    std::vector<ReordererRunner::OutputType> * outputs_;
    OutputCollector * collector_;
    const ConfigRunner& config_;
    HyperGraph * graph_;

    // they are optional
	FeatureSet * bilingual_features_;  // The mapping on feature ids and which to use
	std::string tline_;
	std::string aline_;
};

}
#endif

