/*
 * shift-reduce-trainer.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/shift-reduce-trainer.h"
#include "shift-reduce-dp/parser.h"
#include <boost/foreach.hpp>
#include <time.h>
#include <cstdlib>
using namespace std;

namespace lader {

ShiftReduceTrainer::ShiftReduceTrainer() : learning_rate_(1),
        attach_(CombinedAlign::ATTACH_NULL_LEFT),
        combine_(CombinedAlign::COMBINE_BLOCKS),
        bracket_(CombinedAlign::ALIGN_BRACKET_SPANS) {

}

ShiftReduceTrainer::~ShiftReduceTrainer() {
    BOOST_FOREACH(std::vector<FeatureDataBase*> vec, data_)
        BOOST_FOREACH(FeatureDataBase* ptr, vec)
            delete ptr;
    if(model_) delete model_;
    if(features_) delete features_;
}

void ShiftReduceTrainer::InitializeModel(const ConfigTrainer & config) {
    srand(time(NULL));
    ofstream model_out(config.GetString("model_out").c_str());
    if(!model_out)
        THROW_ERROR("Must specify a valid model output with -model_out ('"
                        <<config.GetString("model_out")<<"')");
    // Load the model from  a file if it exists, otherwise note features
    if(config.GetString("model_in").length() != 0) {
        std::ifstream in(config.GetString("model_in").c_str());
        if(!in) THROW_ERROR("Couldn't read model from file (-model_in '"
                            << config.GetString("model_in") << "')");
        features_ = FeatureSet::FromStream(in);
        model_ = ReordererModel::FromStream(in);
    } else {
        model_ = new ReordererModel;
        model_->SetMaxTerm(config.GetInt("max_term"));
        model_->SetUseReverse(config.GetBool("use_reverse"));
        features_ = new FeatureSet;
        features_->ParseConfiguration(config.GetString("feature_profile"));
    }
    // Load the other config
    attach_ = config.GetString("attach_null") == "left" ?
                CombinedAlign::ATTACH_NULL_LEFT :
                CombinedAlign::ATTACH_NULL_RIGHT;
    combine_ = config.GetBool("combine_blocks") ?
                CombinedAlign::COMBINE_BLOCKS :
                CombinedAlign::LEAVE_BLOCKS_AS_IS;
    model_->SetCost(config.GetDouble("cost"));
}

void ShiftReduceTrainer::TrainIncremental(const ConfigTrainer & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"));
    if(config.GetString("align_in").length())
        ReadAlignments(config.GetString("align_in"));
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    vector<int> sent_order(data_.size());
        for(int i = 0 ; i < (int)sent_order.size(); i++)
            sent_order[i] = i;
    cerr << "(\".\" == 100 sentences)" << endl;
    Parser p;
    p.SetBeamSize(config.GetInt("beam"));
    p.SetVerbose(verbose);
    string update = config.GetString("update");
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        // Shuffle
        if(config.GetBool("shuffle"))
        	random_shuffle(sent_order.begin(), sent_order.end());
        int done = 0;
        int iter_nedge = 0, iter_nstate = 0;
        BOOST_FOREACH(int sent, sent_order) {
        	if(++done % 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;
        	Parser::Result result;
        	if (verbose >= 1){
        		cerr << "Sentence " << sent << endl;
        		cerr << "Rank: ";
        		BOOST_FOREACH(int rank, ranks_[sent].GetRanks())
        			cerr << rank << " ";
        		cerr << endl;
        	}
        	vector<DPState::Action> refseq = ranks_[sent].GetReference();
        	if (verbose >= 1){
        		cerr << "Reference ";
        		BOOST_FOREACH(DPState::Action action, refseq)
        			cerr << action << " ";
        		cerr << endl;
        	}
        	if (refseq.size() < data_[sent][0]->GetNumWords()*2 - 1){
        		if (verbose >= 1)
        			cerr << "Fail to get correct reference sequence, skip it" << endl;
        		continue;
        	}
        	p.Search(*model_, *features_, data_[sent], result, &refseq, update.empty() ? NULL : &update);
        	if (verbose >= 1){
        		cerr << "Purmutation:";
        		BOOST_FOREACH(int order, result.order)
        			cerr << " " << order;
        		cerr << endl;
        	}
        	int step;
        	for (step = 0 ; step < result.step ; step++)
        		if (result.actions[step] != refseq[step])
        			break;
        	FeatureMapInt feat_map;
        	p.Simulate(*model_, *features_, refseq, data_[sent], step, feat_map, +1); // positive examples
        	p.Simulate(*model_, *features_, result.actions, data_[sent], step, feat_map, -1); // negative examples
        	FeatureVectorInt deltafeats;
        	ClearAndSet(deltafeats, feat_map);
        	if(config.GetString("learner") == "pegasos")
        		model_->AdjustWeightsPegasos(deltafeats);
        	else if(config.GetString("learner") == "perceptron")
        		model_->AdjustWeightsPerceptron(deltafeats);
        	else
        		THROW_ERROR("Bad learner: " << config.GetString("learner"));
        	iter_nedge += p.GetNumEdges();
        	iter_nstate += p.GetNumStates();
        }
        cout << "Finished iteration " << iter << ": " << iter_nedge << " edges, " << iter_nstate << " states" << endl;
        cout.flush();
        // write every iteration to a different file
        if(config.GetBool("write_every_iter")){
        	stringstream ss;
        	ss << ".it" << iter;
        	WriteModel(config.GetString("model_out") + ss.str());
        }
        if(iter == config.GetInt("iterations") - 1) {
            WriteModel(config.GetString("model_out"));
            break;
        }
    }
}

void ShiftReduceTrainer::ReadAlignments(const std::string & align_in) {
    std::ifstream in(align_in.c_str());
    if(!in) THROW_ERROR("Could not open alignment file (-align_in): "
                            <<align_in);
    std::string line;
    int i = 0;
    while(getline(in, line))
        ranks_.push_back(
            Ranks(CombinedAlign(SafeAccess(data_,i++)[0]->GetSequence(),
                                Alignment::FromString(line),
                                attach_, combine_, bracket_)));
}

void ShiftReduceTrainer::ReadParses(const std::string & parse_in) {
    std::ifstream in(parse_in.c_str());
    if(!in) THROW_ERROR("Could not open parse file (-parse_in): " << parse_in);
    std::string line;
    while(getline(in, line)) {
        parses_.push_back(FeatureDataParse());
        parses_.rbegin()->FromString(line);
    }
}
} /* namespace lader */
