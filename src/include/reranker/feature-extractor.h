/*
 * feature-extractor.h
 *
 *  Created on: Feb 19, 2014
 *      Author: leona
 */

#ifndef FEATURE_EXTRACTOR_H_
#define FEATURE_EXTRACTOR_H_

#include <lader/feature-data-sequence.h>
#include <reranker/features.h>
#include <reranker/config-extractor.h>
#include <reranker/reranker-model.h>
#include <reranker/flat-tree.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <vector>
#include <shift-reduce-dp/dparser.h>

using namespace std;
using namespace boost;
using namespace lader;

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

    	string kbest_in = config.GetString("kbest_in");
    	ifstream kin(kbest_in.c_str());
    	if (!kin)
    		cerr << "use stdin for kbest_in" << endl;
    	int verbose = config.GetInt("verbose");

    	ReadModel(config);
    	int count = ReadGold(config);
    	cout << "S=" << count << endl;
    	int id = 0;
    	for (int sent = 0 ; getline(kin != NULL? kin : cin, line) ; sent++){
        	if(sent && sent % 1000 == 0) cerr << ".";
        	if(sent && sent % (1000*10) == 0) cerr << sent << endl;
        	int numParses;
        	istringstream iss(line);
        	iss >> numParses;

        	getline(sin, line);
    		FeatureDataSequence words;
    		words.FromString(line);

    		Node * gold = golds_[sent];
    		if (!gold){
				if (verbose >= 1)
					cerr << "Fail to get correct reference sequence, skip " << sent << endl;
				for (int k = 0 ; k < numParses ; k++)
					getline(kin != NULL? kin : cin, line);
				continue;
    		}
        	cout << "G=" << gold->NumEdges() << " N=" << numParses;
    		for (int k = 0 ; k < numParses ; k++) {
    			getline(kin != NULL? kin : cin, line);
    			if (line.empty())
    				THROW_ERROR("Less than " << numParses << " trees" << endl);
    			// Load the data
    			vector<string> columns;
    			algorithm::split(columns, line, is_any_of("\t"));
    			if (columns.size() != 2)
    				THROW_ERROR("Expect score<TAB>kbest, get: " << line << endl);
    			// Load the data
    			double score = atof(columns[0].c_str());
    			ActionVector refseq = DPState::ActionFromString(columns[1].c_str());
    			if (verbose >= 1){
    				cerr << k+1 << " th best:";
					BOOST_FOREACH(DPState::Action action, refseq)
						cerr << " " << (char)action;
					cerr << endl;
    			}
    			Parser * p;
    			if (config.GetInt("max_swap") > 0)
    				p = new DParser(config.GetInt("max_swap"));
    			else
    				p = new DParser;
    			DPState * goal = p->GuidedSearch(refseq, words.GetNumWords());
				if (goal == NULL)
					THROW_ERROR(k << "th best " << columns[1].c_str() << endl
							<< "Cannot guide with max_swap " << config.GetInt("max_swap") << endl)
    			DPStateNode * root = goal->ToFlatTree();
    			delete p;
    			cout << " P=" << root->NumEdges() << " W=" << Node::Intersection(gold, root) << " ";
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
    				f->Extract(root, words);
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
    			BOOST_FOREACH(TreeFeature * f, features){
    				delete f;
    			}
    			delete root;
    		}
    		cout << endl;
    	}
    	if (kin) kin.close();

    	if (config.GetString("model_out").length()){
    		string model_out = config.GetString("model_out");
    		ofstream out(model_out.c_str());
    		model_->ToStream(out, config.GetInt("threshold"));
    		out.close();
    	}
    }

    int ReadGold(const ConfigBase & config){
    	string gold_in = config.GetString("gold_in");
    	std::ifstream in(gold_in.c_str());
		if(!in) THROW_ERROR("Could not open gold file: " <<gold_in);
		std::string line;
		golds_.clear();
		int count = 0;
		string source_in = config.GetString("source_in");
		ifstream sin(source_in.c_str());
		if(!sin) THROW_ERROR("Could not open source file: " <<source_in);
		for (int i = 0 ; getline(in, line) ; i++){
			vector<string> columns;
			algorithm::split(columns, line, is_any_of("\t"));
			// Load the data
			ActionVector refseq = DPState::ActionFromString(columns[0].c_str());
			getline(sin, line);
			FeatureDataSequence sent;
			sent.FromString(line);
			if (config.GetInt("verbose") >= 1){
				cerr << "Sentence " << i << endl;
				cerr << "Source: " << sent.ToString() << endl;
				cerr << "Reference:";
				BOOST_FOREACH(DPState::Action action, refseq)
					cerr << " " << (char)action;
				cerr << endl;
			}

			if (refseq.empty()){
				golds_.push_back(NULL);
				continue;
			}

			Parser * p;
			if (config.GetInt("max_swap") > 0)
				p = new DParser(config.GetInt("max_swap"));
			else
				p = new DParser;
			DPState * goal = p->GuidedSearch(refseq, sent.GetNumWords());
			if (goal){
				DPStateNode * root;
				golds_.push_back(goal->ToFlatTree());
				count++;
			}
			else{
				if (config.GetInt("verbose") >= 1)
					cerr << "Cannot guide with max_swap " << config.GetInt("max_swap") << endl;
				golds_.push_back(NULL);
			}
			delete p;
		}
		return count;
    }

    void ReadModel(const ConfigBase & config){
    	string model_in = config.GetString("model_in");
    	int max_swap = config.GetInt("max_swap");
    	if (model_in.empty()){ // for training
    		model_ = new RerankerModel;
    		model_->SetMaxSwap(max_swap);
    	}
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


#endif /* FEATURE_EXTRACTOR_H_ */
