/*
 * reranker-runner.h
 *
 *  Created on: Feb 27, 2014
 *      Author: leona
 */

#ifndef RERANKER_RUNNER_H_
#define RERANKER_RUNNER_H_

#include <lader/feature-data-sequence.h>
#include <reranker/features.h>
#include <reranker/config-extractor.h>
#include <reranker/reranker-model.h>
#include <reranker/flat-tree.h>
#include <boost/algorithm/string.hpp>
#include <shift-reduce-dp/dparser.h>
#include <fstream>
#include <vector>

using namespace std;
using namespace boost;

namespace reranker{

class RerankerResult {
public:
	double score;
	GenericNode * tree;
	bool operator < (const RerankerResult & rhs) const{
		return score < rhs.score;
	}
};

class RerankerRunner{

public:
	virtual ~RerankerRunner(){
		delete model_;
	}
    void Run(const ConfigBase & config) {
    	string line;
    	string source_in = config.GetString("source_in");
    	ifstream sin(source_in.c_str());

    	string kbest_in = config.GetString("kbest_in");
    	ifstream kin(kbest_in.c_str());
    	if (!kin)
    		cerr << "use stdin for kbest_in" << endl;
    	int verbose = config.GetInt("verbose");
    	int K = config.GetInt("trees");

    	ReadModelAndWeights(config.GetString("model_in"), config.GetString("weights"));
    	for (int sent = 0 ; getline(kin != NULL? kin : cin, line) ; sent++){
        	if(sent && sent % 1000 == 0) cerr << ".";
        	if(sent && sent % (1000*10) == 0) cerr << sent << endl;
        	int numParses;
        	istringstream iss(line);
        	iss >> numParses;
        	if (verbose >= 1)
        		cerr << "Sentence " << sent << ": " << numParses << " parses" << endl;
        	getline(sin, line);
    		FeatureDataSequence words;
    		words.FromString(line);
        	vector<RerankerResult> buf(numParses);
    		for (int i = 0 ; i < numParses ; i++) {
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
    			vector<DPState::Action> refseq = DPState::ActionFromString(columns[1].c_str());
				Parser * p;
				if (model_->GetMaxSwap() > 0)
					p = new DParser(model_->GetMaxSwap());
				else
					p = new DParser;
				DPState * goal = p->GuidedSearch(refseq, words.GetNumWords());
				DPStateNode * root = goal->ToFlatTree();
				delete p;
    			buf[i].tree = root;
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
    			buf[i].score = model_->GetScore(featmap);
    			BOOST_FOREACH(TreeFeature * f, features){
    				delete f;
    			}
    		}
    		if (verbose >= 1){
    			BOOST_FOREACH(RerankerResult r, buf){
    				cerr << r.score << "\t";
    				r.tree->PrintParse(words.GetSequence(), cerr);
    				cerr << endl;
    			}
    		}
    		// Get the reranking result
    		RerankerResult * best;
    		if (K > 0 && K < numParses){
    			sort(buf.begin(), buf.begin()+K);
    			best = &buf[K-1];
    		}else{
    			sort(buf.begin(), buf.end());
    			best = &buf.back();
    		}
    		vector<int> order;
    		best->tree->GetOrder(order);
    		words.Reorder(order);
    		int i = 0;
    		BOOST_FOREACH(int o, order){
    			if (i++ != 0) cout << " ";
    			cout << o;
    		}
    		cout << "\t" << words.ToString() << "\t" << best->score << endl;
    		BOOST_FOREACH(RerankerResult r, buf){
    			delete r.tree;
    		}
    	}
    	if (sin) sin.close();
    }

    void ReadModelAndWeights(string  model_in, string weights){
		std::ifstream in(model_in.c_str());
		if(!in) THROW_ERROR("Could not open model file: " <<model_in);
		model_ = RerankerModel::FromStream(in);
		model_->SetAdd(false);
		std::ifstream win(weights.c_str());
		if(!win) THROW_ERROR("Could not open weight file: " <<weights);
		model_->SetWeightsFromStream(win);
		if (model_->GetWeights().size() != model_->GetFeatureIds().size())
			THROW_ERROR("Different size weights " << model_->GetWeights().size()
					<< " and features " << model_->GetFeatureIds().size() << endl);
		win.close();
    }
private:
    RerankerModel * model_;
};

}


#endif /* RERANKER_RUNNER_H_ */
