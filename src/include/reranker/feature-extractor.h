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
#include <reranker/flat-tree.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <vector>

using namespace std;
using namespace boost;

namespace reranker{

class FeatureExtractor{
//	// A task
//	class ExtractFeatureTask : public Task {
//	public:
//	    ExtractFeatureTask(int id, const string & line,
//	                  const ConfigExtractor& config,
//	                  RerankerModel & model,
//	                  OutputCollector & collector) :
//	        id_(id), line_(line), config_(config),
//	        model_(model), collector_(collector) { }
//	    void Run(){
//    		ostringstream oss, ess;
//	    	int verbose = config_.GetInt("verbose");
//	    	if (verbose > 0)
//	    		ess << line_ << endl;
//		    // Load the data
//		    vector<string> columns;
//		    algorithm::split(columns, line_, is_any_of("\t"));
//		    if (columns.size() == 1){
//		    	oss << line_ << endl;
//		    	collector_.Write(id_, oss.str(), ess.str());
//		    	return;
//		    }
//		    else if (columns.size() != 2)
//		    	THROW_ERROR("Expect score<TAB>flatten, get: " << line_ << endl);
//		    // Load the data
//		    double score = atof(columns[0].c_str());
//		    GenericNode::ParseResult * result = GenericNode::ParseInput(columns[1].c_str());
//		    FeatureDataSequence sent(result->words);
//		    if (verbose > 0)
//		    	cerr << sent.GetNumWords() << ": " << sent.ToString() << endl;
//		    // Extract and set features
//		    FeatureMapInt featmap;
//		    NLogP f0(score);
//		    f0.Set(featmap, model_);
//	    	TreeFeatureSet features;
//	    	features.push_back(new Rule);
//	    	features.push_back(new Word(config_.GetInt("level")));
//	    	features.push_back(new NGram(config_.GetInt("order")));
//	    	features.push_back(new NGramTree(config_.GetInt("order")));
//		    BOOST_FOREACH(TreeFeature * f, features){
//		    	f->SetLexical(config_.GetBool("lexicalize-parent"), config_.GetBool("lexicalize-child"));
//		    	f->Extract(result->root, sent);
//		    	f->Set(featmap, model_);
//		    }
//		    int i = 0;
//		    BOOST_FOREACH(FeaturePairInt fp, featmap){
//		    	if (i++ != 0) oss << " ";
//		    	oss << fp.first << ":" << fp.second;
//		    }
//		    oss << endl;
//		    collector_.Write(id_, oss.str(), ess.str());
//		    delete result->root;
//		    delete result;
//	    	BOOST_FOREACH(TreeFeature * f, features)
//	    		delete f;
//	    }
//	protected:
//	    int id_;
//	    const string line_;
//	    const ConfigExtractor& config_;
//	    RerankerModel & model_;
//	    OutputCollector & collector_;
//	};
public:
	virtual ~FeatureExtractor(){
		BOOST_FOREACH(Node * node, golds_)
			if (node)
				delete node;
	}
    // Run the model
    void Run(const ConfigBase & config) {
    	// Create the thread pool
//    	ThreadPool pool(config.GetInt("threads"), 1000);
    	OutputCollector collector;

    	string line;
    	string source_in = config.GetString("source_in");
    	ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int verbose = config.GetInt("verbose");

    	string gold_in = config.GetString("gold_in");
    	int count = ReadGold(gold_in);
    	cout << "S=" << count << endl;
    	RerankerModel model;
    	int id = 0;
    	for (int sent = 0 ; getline(sin != NULL? sin : cin, line) ; sent++){
//    		ExtractFeatureTask *task = new ExtractFeatureTask(
//    				id++, line, config, model, collector);
//    		pool.Submit(task);
        	int numSent;
        	istringstream iss(line);
        	iss >> numSent;
        	Node * gold = golds_[sent];
    		if (!gold){
				if (verbose >= 1)
					cerr << "Fail to get correct reference sequence, skip " << sent << endl;
				for (int i = 0 ; i < numSent ; i++)
					getline(sin != NULL? sin : cin, line);
				continue;
    		}
        	cout << "G=" << gold->NumEdges() << " N=" << numSent;
    		for (int i = 0 ; i < numSent ; i++) {
    			getline(sin != NULL? sin : cin, line);
    			if (line.empty())
    				THROW_ERROR("Less than " << numSent << " trees" << endl);
    			// Load the data
    			vector<string> columns;
    			algorithm::split(columns, line, is_any_of("\t"));
    			if (columns.size() != 2)
    				THROW_ERROR("Expect score<TAB>flatten, get: " << line << endl);
    			// Load the data
    			double score = atof(columns[0].c_str());
    			GenericNode::ParseResult * result = GenericNode::ParseInput(columns[1].c_str());
    			cout << " P=" << result->root->NumEdges() << " W=" << Node::Intersection(gold, result->root) << " ";
    			FeatureDataSequence sent(result->words);
    			if (verbose > 0)
    				cerr << sent.GetNumWords() << ": " << sent.ToString() << endl;
    			// Extract and set features
    			FeatureMapInt featmap;
    			NLogP f0(score);
    			f0.Set(featmap, model);
    			TreeFeatureSet features;
    			features.push_back(new Rule());
    			features.push_back(new Word(config.GetInt("level")));
    			features.push_back(new NGram(config.GetInt("order")));
    			features.push_back(new NGramTree(config.GetInt("order")));
    			BOOST_FOREACH(TreeFeature * f, features){
    				f->SetLexical(config.GetBool("lexicalize-parent"), config.GetBool("lexicalize-child"));
    				f->Extract(result->root, sent);
    				f->Set(featmap, model);
    			}
    			int i = 0;
    			BOOST_FOREACH(FeaturePairInt fp, featmap){
    				if (i++ != 0) cout << " ";
    				if (fp.second != 1)
    					cout << fp.first << "=" << fp.second;
    				else
    					cout << fp.first;
    			}
    			cout << ",";
    			delete result->root;
    			delete result;
    			BOOST_FOREACH(TreeFeature * f, features){
    				delete f;
    			}
    		}
    		cout << endl;
        	if(sent % 1000 == 0) cerr << ".";
        	if(sent % (1000*10) == 0) cerr << sent << endl;
    	}
    	if (sin) sin.close();
//    	pool.Stop(true);

    	string model_out = config.GetString("model_out");
    	ofstream out(model_out.c_str());
    	model.ToStream(out, config.GetInt("threshold"));
    	out.close();
    }

    int ReadGold(string & gold_in){
    	std::ifstream in(gold_in.c_str());
		if(!in) THROW_ERROR("Could not open gold file: " <<gold_in);
		std::string line;
		golds_.clear();
		int count = 0;
		while(getline(in, line)){
			GenericNode::ParseResult * result = GenericNode::ParseInput(line);
			if (result){
				golds_.push_back(result->root);
				count++;
			}
			else
				golds_.push_back(NULL);
		}
		return count;
    }
private:
    vector<Node*> golds_;
};

}


#endif /* RERANKER_TRAINER_H_ */
