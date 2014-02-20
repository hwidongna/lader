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
#include <reranker/read-tree.h>
#include <reranker/config-trainer.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
using namespace std;
using namespace boost;

namespace reranker{

class RerankerTrainer{
public:
    // Run the model
    void Run(const ConfigTrainer & config) {
    	// Create the thread pool
    	OutputCollector collector;
    	TreeFeatureSet features;
    	features.push_back(new Rule);
    	features.push_back(new Word(config.GetInt("level")));
    	features.push_back(new NGram(config.GetInt("order")));
    	features.push_back(new NGramTree(config.GetInt("order")));
    	BOOST_FOREACH(TreeFeature * f, features)
    		f->SetLexicalized(config.GetBool("lexicalize-parent"),
    						config.GetBool("lexicalize-child"));

    	string line;
    	string source_in = config.GetString("source_in");
    	ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int id = 0;
    	SymbolSet<int> symbols;
    	while(getline(sin != NULL? sin : cin, line)) {
    		ostringstream oss, ess;
	    	int verbose = config.GetInt("verbose");
	    	if (verbose > 0)
	    		ess << line << endl;
		    // Load the data
		    vector<string> columns;
		    algorithm::split(columns, line, is_any_of("\t"));
		    if (columns.size() == 1){
		    	oss << line << endl;
		    	collector.Write(id++, oss.str(), ess.str());
		    	continue;
		    }
		    else if (columns.size() != 2)
		    	THROW_ERROR("Expect score<TAB>flatten, get: " << line << endl);
		    // Load the data
		    GenericNode dummy('R');
		    double score = atof(columns[0].c_str());
		    ReadTreeResult * result = ParseInput(columns[1].c_str());
		    GenericNode * root = result->root;
		    dummy.AddChild(root);
		    FeatureDataSequence sent(result->words);
		    if (verbose > 0)
		    	cerr << sent.GetNumWords() << ": " << sent.ToString() << endl;
		    // Extract and set features
		    FeatureMapInt featmap;
		    NLogP f0(score);
		    f0.Set(featmap, symbols);
		    BOOST_FOREACH(TreeFeature * f, features){
		    	f->SetLexicalized(config.GetBool("lexicalize-parent"), config.GetBool("lexicalize-child"));
		    	f->Extract(root, sent);
		    	f->Set(featmap, symbols);
		    }
		    int i = 0;
		    BOOST_FOREACH(FeaturePairInt fp, featmap){
		    	if (i++ != 0) oss << " ";
		    	oss << fp.first << ":" << fp.second;
		    }
		    oss << endl;
		    collector.Write(id++, oss.str(), ess.str());
		    delete result;
    	}
    	if (sin) sin.close();
    	BOOST_FOREACH(TreeFeature * f, features)
    		delete f;
    }
};

}


#endif /* RERANKER_TRAINER_H_ */
