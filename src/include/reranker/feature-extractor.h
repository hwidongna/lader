/*
 * reranker-trainer.h
 *
 *  Created on: Feb 19, 2014
 *      Author: leona
 */

#ifndef RERANKER_TRAINER_H_
#define RERANKER_TRAINER_H_

#include <lader/thread-pool.h>
#include <lader/symbol-set.h>
#include <lader/feature-vector.h>
#include <lader/feature-data-sequence.h>
#include <lader/output-collector.h>
#include <reranker/features.h>
#include <reranker/config-extractor.h>
#include <reranker/reranker-model.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
using namespace std;
using namespace boost;

namespace reranker{

class FeatureExtractor{
	// A task
	class ExtractFeatureTask : public Task {
	public:
	    ExtractFeatureTask(int id, const string & line,
	                  const ConfigExtractor& config,
	                  RerankerModel & model,
	                  OutputCollector & collector) :
	        id_(id), line_(line), config_(config),
	        model_(model), collector_(collector) { }
	    void Run(){
    		ostringstream oss, ess;
	    	int verbose = config_.GetInt("verbose");
	    	if (verbose > 0)
	    		ess << line_ << endl;
		    // Load the data
		    vector<string> columns;
		    algorithm::split(columns, line_, is_any_of("\t"));
		    if (columns.size() == 1){
		    	oss << line_ << endl;
		    	collector_.Write(id_, oss.str(), ess.str());
		    	return;
		    }
		    else if (columns.size() != 2)
		    	THROW_ERROR("Expect score<TAB>flatten, get: " << line_ << endl);
		    // Load the data
		    double score = atof(columns[0].c_str());
		    GenericNode dummy('R');
		    GenericNode::ParseResult * result = dummy.ParseInput(columns[1].c_str());
		    dummy.AddChild(result->root);
		    FeatureDataSequence sent(result->words);
		    if (verbose > 0)
		    	cerr << sent.GetNumWords() << ": " << sent.ToString() << endl;
		    // Extract and set features
		    FeatureMapInt featmap;
		    NLogP f0(score);
		    f0.Set(featmap, model_);
	    	TreeFeatureSet features;
	    	features.push_back(new Rule);
	    	features.push_back(new Word(config_.GetInt("level")));
	    	features.push_back(new NGram(config_.GetInt("order")));
	    	features.push_back(new NGramTree(config_.GetInt("order")));
		    BOOST_FOREACH(TreeFeature * f, features){
		    	f->SetLexical(config_.GetBool("lexicalize-parent"), config_.GetBool("lexicalize-child"));
		    	f->Extract(result->root, sent);
		    	f->Set(featmap, model_);
		    }
		    int i = 0;
		    BOOST_FOREACH(FeaturePairInt fp, featmap){
		    	if (i++ != 0) oss << " ";
		    	oss << fp.first << ":" << fp.second;
		    }
		    oss << endl;
		    collector_.Write(id_, oss.str(), ess.str());
		    delete result;
	    	BOOST_FOREACH(TreeFeature * f, features)
	    		delete f;
	    }
	protected:
	    int id_;
	    const string line_;
	    const ConfigExtractor& config_;
	    RerankerModel & model_;
	    OutputCollector & collector_;
	};
public:
    // Run the model
    void Run(const ConfigExtractor & config) {
    	// Create the thread pool
    	ThreadPool pool(config.GetInt("threads"), 1000);
    	OutputCollector collector;

    	string line;
    	string source_in = config.GetString("source_in");
    	ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int id = 0;
    	RerankerModel model;
    	int done = 0;
    	while(getline(sin != NULL? sin : cin, line)) {
        	if(++done % 1000 == 0) cerr << ".";
        	if(done % (1000*10) == 0) cerr << done << endl;
    		ExtractFeatureTask *task = new ExtractFeatureTask(
    				id++, line, config, model, collector);
    		pool.Submit(task);
    	}
    	if (sin) sin.close();
    	pool.Stop(true);

    	string model_out = config.GetString("model_out");
    	ofstream out(model_out.c_str());
    	model.ToStream(out, config.GetInt("threshold"));
    	out.close();
    }
};

}


#endif /* RERANKER_TRAINER_H_ */
