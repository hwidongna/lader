/*
 * shift-reduce-runner.h
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */

#ifndef SHIFT_REDUCE_KBEST_H_
#define SHIFT_REDUCE_KBEST_H_

#include <lader/reorderer-runner.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/parser.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>

using namespace std;
namespace lader {


class ShiftReduceKbest : public ReordererRunner{
	// A task
	class ShiftReduceTask : public Task {
	public:
	    ShiftReduceTask(int id, const string & line,
	                  ReordererModel * model, FeatureSet * features,
	                  vector<ReordererRunner::OutputType> * outputs,
	                  const ConfigRunner& config, OutputCollector * collector) :
	        id_(id), line_(line), model_(model), features_(features),
	        outputs_(outputs), collector_(collector), config_(config) { }
	    void Run(){
	    	int verbose = config_.GetInt("verbose");
	    	int sent = id_;
		    // Load the data
		    Sentence datas = features_->ParseInput(line_);
		    // Save the original string
		    vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
		    ostringstream oss, ess;
		    // Build a reordering tree
		    if (verbose >= 1)
		    	ess << endl << "Sentence " << sent << endl;
	    	Parser p;
			p.SetBeamSize(config_.GetInt("beam"));
			p.SetVerbose(config_.GetInt("verbose"));
			Parser::Result result;
			p.Search(*model_, *features_, datas, result, config_.GetInt("max_state"));
			vector<Parser::Result> kbest;
			p.GetKbestResult(kbest);
			oss << kbest.size() << endl;
			for (int k = 0 ; k < kbest.size() ; k++){
				Parser::Result & result = kbest[k];
				if (verbose >= 1){
					ess << "Result:   ";
					for (int step = 0 ; step < (const int)result.actions.size() ; step++)
						ess << " " << result.actions[step];
					ess << endl;
					ess << "Purmutation:";
					BOOST_FOREACH(int order, result.order)
					ess << " " << order;
					ess << endl;
				}
				// Reorder
				std::vector<int> & reordering = result.order;
				datas[0]->Reorder(reordering);
				// Print the string
				for(int i = 0; i < (int)outputs_->size(); i++) {
					if(i != 0) oss << "\t";
					if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
						oss << datas[0]->ToString();
					} else if(outputs_->at(i) == ReordererRunner::OUTPUT_PARSE) {
						p.GetKthBest(k)->PrintParse(words, oss);
					} else if(outputs_->at(i) == ReordererRunner::OUTPUT_ORDER) {
						for(int j = 0; j < (int)reordering.size(); j++) {
							if(j != 0) oss << " ";
							oss << reordering[j];
						}
					} else if(outputs_->at(i) == ReordererRunner::OUTPUT_SCORE) {
						oss << result.score;
					} else if(outputs_->at(i) == ReordererRunner::OUTPUT_FLATTEN) {
						reranker::DPStateNode dummy(0, words.size(), NULL, DPState::NOP);
						reranker::DPStateNode * root = dummy.Flatten(p.GetKthBest(k));
						root->PrintParse(words, oss);
					} else {
						THROW_ERROR("Unimplemented output format");
					}
				}
				oss << endl;
			}
		    collector_->Write(id_, oss.str(), ess.str());
		    // Clean up the data
		    BOOST_FOREACH(FeatureDataBase* data, datas)
		        delete data;
	    }
	protected:
	    int id_;
	    string line_;
	    ReordererModel * model_; // The model
	    FeatureSet * features_;  // The mapping on feature ids and which to use
	    vector<ReordererRunner::OutputType> * outputs_;
	    OutputCollector * collector_;
	    const ConfigRunner& config_;
	};
public:
    // Run the model
    void Run(const ConfigRunner & config) {
    	InitializeModel(config);
    	// Create the thread pool
    	ThreadPool pool(config.GetInt("threads"), 1000);
    	OutputCollector collector;

    	std::string line;
    	std::string source_in = config.GetString("source_in");
    	std::ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int id = 0;
    	// do not need to set threads because it runs in parallel at sentence-level
    	while(std::getline(sin != NULL? sin : std::cin, line)) {
    		ShiftReduceTask *task = new ShiftReduceTask(id++, line, model_, features_,
    				&outputs_, config, &collector);
    		pool.Submit(task);
    	}
    	if (sin) sin.close();
    	pool.Stop(true);
    }
};

} /* namespace lader */
#endif /* SHIFT_REDUCE_KBEST_H_ */
