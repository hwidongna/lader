/*
 * shift-reduce-runner.h
 *
 *  Created on: Dec 29, 2013
 *      Author: leona
 */

#ifndef SHIFT_REDUCE_RUNNER_H_
#define SHIFT_REDUCE_RUNNER_H_

#include <lader/reorderer-runner.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/dparser.h>
#include <shift-reduce-dp/shift-reduce-model.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>

using namespace std;
namespace lader {


// A task
class ShiftReduceTask : public Task {
public:
	ShiftReduceTask(int id, const string & line,
			ShiftReduceModel * model, FeatureSet * features,
			vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				id_(id), line_(line), model_(model), features_(features),
				outputs_(outputs), collector_(collector), config_(config) { }
	void Output(const Sentence & datas, const DPState *best)
    {
        Parser::Result result;
        SetResult(&result, best);
        if (config_.GetInt("verbose") >= 1){
			ess << "Result:   ";
			for (int step = 0 ; step < (const int)result.actions.size() ; step++)
				ess << " " << (char) result.actions[step];
			ess << endl;
			ess << "Purmutation:";
			BOOST_FOREACH(int order, result.order)
				ess << " " << order;
			ess << endl;
		}
        // Reorder
        const std::vector<int> & reordering = result.order;
        // Print the string
        vector<string> words = ((FeatureDataSequence*)(datas[0]))->GetSequence();
        if (result.order.size() != words.size()){
        	ess << "Invalid parsing result " << endl;
        	best->PrintTrace(ess);
        	oss << endl;
        	return;
        }
        for(int i = 0; i < (int)outputs_->size(); i++) {
			if(i != 0) oss << "\t";
			if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
				for(int j = 0; j < (int)reordering.size(); j++) {
					if(j != 0) oss << " ";
					oss << words[reordering[j]];
				}
			} else if(outputs_->at(i) == ReordererRunner::OUTPUT_PARSE) {
				best->PrintParse(words, oss);
			} else if(outputs_->at(i) == ReordererRunner::OUTPUT_ORDER) {
				for(int j = 0; j < (int)reordering.size(); j++) {
					if(j != 0) oss << " ";
					oss << reordering[j];
				}
			} else if(outputs_->at(i) == ReordererRunner::OUTPUT_SCORE) {
				oss << result.score;
			} else if(outputs_->at(i) == ReordererRunner::OUTPUT_FLATTEN) {
				reranker::DDPStateNode root(0, words.size(), NULL, DPState::INIT);
				root.AddChild(root.Flatten(best));
				root.PrintParse(words, oss);
			} else if(outputs_->at(i) == ReordererRunner::OUTPUT_ACTION) {
				for(int j = 0; j < (int)result.actions.size(); j++) {
					if(j != 0) oss << " ";
					oss << (char) result.actions[j];
				}
			} else {
				THROW_ERROR("Unimplemented output format");
			}
		}
        oss << endl;
    }

    void Run()
    {
        int sent = id_;
        // Load the data
        Sentence datas = features_->ParseInput(line_);
        // Save the original string
        // Build a reordering tree
        if(config_.GetInt("verbose") >= 1)
            ess << endl << "Sentence " << sent << endl;

        Parser *p;
        if(model_->GetMaxSwap() > 0)
            p = new DParser(model_->GetMaxSwap());

        else
            p = new Parser();

        p->SetBeamSize(config_.GetInt("beam"));
        p->SetVerbose(config_.GetInt("verbose"));
        p->Search(*model_, *features_, datas);
        DPState *best = p->GetBest();
        Output(datas, best);
		collector_->Write(id_, oss.str(), ess.str());
		// Clean up the data
		BOOST_FOREACH(FeatureDataBase* data, datas){
			delete data;
		}
		delete p;
	}
protected:
    ostringstream oss, ess;
	int id_;
	string line_;
	ShiftReduceModel * model_; // The model
	FeatureSet * features_;  // The mapping on feature ids and which to use
	vector<ReordererRunner::OutputType> * outputs_;
	OutputCollector * collector_;
	const ConfigBase& config_;
};

class ShiftReduceRunner : public ReordererRunner{
public:
	// Initialize the model
	void InitializeModel(const ConfigBase & config) {
		std::ifstream in(config.GetString("model").c_str());
		if(!in) THROW_ERROR("Couldn't read model from file (-model '"
							<< config.GetString("model") << "')");
		features_ = FeatureSet::FromStream(in);
		model_ = ShiftReduceModel::FromStream(in);
	    model_->SetAdd(false);
	    tokenizer<char_separator<char> > outs(config.GetString("out_format"),
	                                          char_separator<char>(","));
	    BOOST_FOREACH(const string & str, outs) {
	        if(str == "string")
	            outputs_.push_back(OUTPUT_STRING);
	        else if(str == "parse")
	            outputs_.push_back(OUTPUT_PARSE);
	        else if(str == "hypergraph")
	            outputs_.push_back(OUTPUT_HYPERGRAPH);
	        else if(str == "order")
	            outputs_.push_back(OUTPUT_ORDER);
	        else if(str == "score")
	        	outputs_.push_back(OUTPUT_SCORE);
	        else if(str == "flatten")
	        	outputs_.push_back(OUTPUT_FLATTEN);
	        else if (str == "action")
	        	outputs_.push_back(OUTPUT_ACTION);
	        else
	            THROW_ERROR("Bad output format '" << str <<"'");
	    }
	}
	virtual Task * NewShiftReduceTask(int id, const string & line,
			const ConfigBase& config) {
		return new ShiftReduceTask(id, line,
				dynamic_cast<ShiftReduceModel*>(model_), features_, &outputs_,
				config, &collector_);
	}
    // Run the model
    void Run(const ConfigBase & config) {
    	InitializeModel(config);
    	// Create the thread pool
    	ThreadPool pool(config.GetInt("threads"), 1000);

    	std::string line;
    	std::string source_in = config.GetString("source_in");
    	std::ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int id = 0;
    	// do not need to set threads because it runs in parallel at sentence-level
    	while(std::getline(sin != NULL? sin : std::cin, line)) {
    		Task *task = NewShiftReduceTask(id++, line, config);
    		pool.Submit(task);
    	}
    	if (sin) sin.close();
    	pool.Stop(true);
    }
};

} /* namespace lader */
#endif /* SHIFT_REDUCE_RUNNER_H_ */
