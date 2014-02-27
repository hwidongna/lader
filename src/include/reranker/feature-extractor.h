/*
 * reranker-trainer.h
 *
 *  Created on: Feb 19, 2014
 *      Author: leona
 */

#ifndef RERANKER_TRAINER_H_
#define RERANKER_TRAINER_H_

#include <lader/feature-data-sequence.h>
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
public:
	virtual ~FeatureExtractor(){
		BOOST_FOREACH(Node * node, golds_)
			if (node)
				delete node;
		delete model_;
	}
    // Run the model
    void Run(const ConfigBase & config) {
    	string line;
    	string source_in = config.GetString("source_in");
    	ifstream sin(source_in.c_str());
    	if (!sin)
    		cerr << "use stdin for source_in" << endl;
    	int verbose = config.GetInt("verbose");

    	ReadModel(config.GetString("model_in"));
    	int count = ReadGold(config.GetString("gold_in"));
    	cout << "S=" << count << endl;
    	int id = 0;
    	for (int sent = 0 ; getline(sin != NULL? sin : cin, line) ; sent++){
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
    			f0.Set(featmap, *model_);
    			TreeFeatureSet features;
    			features.push_back(new Rule());
    			features.push_back(new Word(config.GetInt("level")));
    			features.push_back(new NGram(config.GetInt("order")));
    			features.push_back(new NGramTree(config.GetInt("order")));
    			BOOST_FOREACH(TreeFeature * f, features){
    				f->SetLexical(config.GetBool("lexicalize-parent"), config.GetBool("lexicalize-child"));
    				f->Extract(result->root, sent);
    				f->Set(featmap, *model_);
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

    	if (config.GetString("model_out").length()){
    		string model_out = config.GetString("model_out");
    		ofstream out(model_out.c_str());
    		model_->ToStream(out, config.GetInt("threshold"));
    		out.close();
    	}
    }

    int ReadGold(string  gold_in){
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

    void ReadModel(string  model_in){
    	if (model_in.empty()) // for training
    		model_ = new RerankerModel;
    	else{ // otherwise
    		std::ifstream in(model_in.c_str());
    		if(!in) THROW_ERROR("Could not open model file: " <<model_in);
    		model_ = RerankerModel::FromStream(in);
    		in.close();
    	}
    }
private:
    vector<Node*> golds_;
    RerankerModel * model_;
};

}


#endif /* RERANKER_TRAINER_H_ */
