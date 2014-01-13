/*
 * shift-reduce-trainer.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/shift-reduce-trainer.h"
#include "shift-reduce-dp/parser.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>
#include <cstdlib>
using namespace std;
using namespace boost;

namespace lader {

ShiftReduceTrainer::ShiftReduceTrainer() : learning_rate_(1),
        attach_(CombinedAlign::ATTACH_NULL_LEFT),
        combine_(CombinedAlign::COMBINE_BLOCKS),
        bracket_(CombinedAlign::ALIGN_BRACKET_SPANS) {

}

ShiftReduceTrainer::~ShiftReduceTrainer() {
    BOOST_FOREACH(Sentence * vec, train_data_)
    	if (vec){
    		BOOST_FOREACH(FeatureDataBase* ptr, *vec)
    			delete ptr;
    		delete vec;
    	}
    BOOST_FOREACH(Ranks * ranks, train_ranks_)
    	if (ranks)
        	delete ranks;
    BOOST_FOREACH(Sentence * vec, dev_data_)
		if (vec){
			BOOST_FOREACH(FeatureDataBase* ptr, *vec)
								delete ptr;
			delete vec;
		}
    BOOST_FOREACH(Ranks * ranks, dev_ranks_)
		if (ranks)
			delete ranks;
    BOOST_FOREACH(LossBase * loss, losses_)
    	delete loss;
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
    std::vector<std::string> losses, first_last;
    algorithm::split(
        losses, config.GetString("loss_profile"), is_any_of("|"));
    BOOST_FOREACH(string s, losses) {
        algorithm::split(first_last, s, is_any_of("="));
        if(first_last.size() != 2) THROW_ERROR("Bad loss: " << s);
        LossBase * loss = LossBase::CreateNew(first_last[0]);
        double dub;
        istringstream iss(first_last[1]);
        iss >> dub;
        loss->SetWeight(dub);
        losses_.push_back(loss);
    }
}

template <class T>
int MoveRandom(vector<T> & from, vector<T> & to, double ratio){
	srand(time(0)); // intensionally use same seed across fuction calls
	to.resize(from.size());
	int count = 0;
	for (int i = 0 ; i < from.size() ; i++){
		double r = ((double) rand() / (RAND_MAX));
		if (r > ratio)
			continue;
		to[i] = from[i];
		from[i] = NULL;
		count++;
	}
	if (count == 0)
		return MoveRandom(from, to, ratio); // try again
	return count;
}

void ShiftReduceTrainer::TrainIncremental(const ConfigTrainer & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"), train_data_);
    if(config.GetString("align_in").length())
        ReadAlignments(config.GetString("align_in"), train_ranks_, train_data_);
    if(config.GetString("source_dev").length() && config.GetString("align_dev").length()){
    	ReadData(config.GetString("source_dev"), dev_data_);
    	ReadAlignments(config.GetString("align_dev"), dev_ranks_, dev_data_);
    }
    else{
    	int count1 = MoveRandom(train_data_, dev_data_, config.GetDouble("ratio_dev"));
    	int count2 = MoveRandom(train_ranks_, dev_ranks_, config.GetDouble("ratio_dev"));
    	if (count1 != count2)
    		THROW_ERROR("Do not split train set correctly");
    	cerr << "Split " << train_data_.size() << " train set into "
    			<< train_data_.size()-count1 << " train and " << count1 << " dev set" << endl;
    }
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    vector<int> sent_order(train_data_.size());
	for(int i = 0 ; i < (int)sent_order.size(); i++)
		sent_order[i] = i;

    cerr << "(\".\" == 100 sentences)" << endl;
    Parser p;
    p.SetBeamSize(config.GetInt("beam"));
    p.SetVerbose(verbose);
    string update = config.GetString("update");
    struct timespec search={0,0}, simulate={0,0};
    struct timespec tstart={0,0}, tend={0,0};
    double best_prec = 0;
    int best_iter, best_wlen;
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        // Shuffle
        if(config.GetBool("shuffle"))
        	random_shuffle(sent_order.begin(), sent_order.end());
        int done = 0;
        double iter_nedge = 0, iter_nstate = 0;
        int bad_update = 0, early_update = 0;
        if (verbose >= 1)
        	cerr << "Start training parsing iter " << iter << endl;
        BOOST_FOREACH(int sent, sent_order) {
        	if (!train_ranks_[sent]) // if leaving-one-out, it would be NULL
        		continue;
        	if(++done% 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;

        	if (verbose >= 1){
        		cerr << endl << "Sentence " << sent << endl;
        		cerr << "Rank:";
        		BOOST_FOREACH(int rank, train_ranks_[sent]->GetRanks())
        			cerr << " " << rank;
        		cerr << endl;
        	}
        	vector<DPState::Action> refseq = train_ranks_[sent]->GetReference();
        	if (verbose >= 1){
        		cerr << "Reference:";
        		BOOST_FOREACH(DPState::Action action, refseq)
        			cerr << " " << action;
        		cerr << endl;
        	}
        	if (refseq.size() < (*train_data_[sent])[0]->GetNumWords()*2 - 1){
        		if (verbose >= 1)
        			cerr << "Fail to get correct reference sequence, skip it" << endl;
        		continue;
        	}

        	Parser::Result result;
        	clock_gettime(CLOCK_MONOTONIC, &tstart);
        	p.Search(*model_, *features_, *train_data_[sent], result, config.GetInt("max_state"),
        			&refseq, update.empty() ? NULL : &update);
        	clock_gettime(CLOCK_MONOTONIC, &tend);
        	search.tv_sec += tend.tv_sec - tstart.tv_sec;
        	search.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
        	if (result.step != result.actions.size())
        		THROW_ERROR(result << endl);

        	if (verbose >= 1){
        		cerr << "Result:   ";
				for (int step = 0 ; step < (const int)result.actions.size() ; step++)
					cerr << " " << result.actions[step];
				cerr << endl;
        	}
        	if (result.step < refseq.size())
        		early_update++;
        	refseq.resize(result.step);
        	int step;
        	for (step = 1 ; step <= result.step ; step++)
        		if (result.actions[step-1] != refseq[step-1])
        			break;
        	if (verbose >= 1 && result.step >= step)
        		cerr << "Result step " << result.step << ", update from " << step << endl;
        	clock_gettime(CLOCK_MONOTONIC, &tstart);
        	FeatureMapInt feat_map;
        	p.Simulate(*model_, *features_, refseq, *train_data_[sent], step, feat_map, +1); // positive examples
        	p.Simulate(*model_, *features_, result.actions, *train_data_[sent], step, feat_map, -1); // negative examples
        	clock_gettime(CLOCK_MONOTONIC, &tend);
        	simulate.tv_sec += tend.tv_sec - tstart.tv_sec;
			simulate.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
        	FeatureVectorInt deltafeats;
        	ClearAndSet(deltafeats, feat_map);
        	if (verbose >= 2){
        		cerr << "Delta feats" << endl;
        		FeatureVectorString * fvs = model_->StringifyFeatureVector(deltafeats);
				BOOST_FOREACH(FeaturePairString feat, *fvs)
					cerr << feat.first << ":" << feat.second << " ";
				cerr << endl;
				delete fvs;
        	}
        	if (model_->ScoreFeatureVector(deltafeats) > 0){
        		bad_update++;
        		if (verbose >= 1)
        			cerr << "Bad update at step " << result.step << endl;
        	}
        	model_->AdjustWeightsPerceptron(deltafeats);
        	iter_nedge += p.GetNumEdges();
        	iter_nstate += p.GetNumStates();
        }
        cerr << "Running time on average: "
        		<< "searh " << ((double)search.tv_sec + 1.0e-9*search.tv_nsec) / (iter+1) << "s"
        		<< ", simulate " << ((double)simulate.tv_sec + 1.0e-9*simulate.tv_nsec) / (iter+1) << "s" << endl;
        time_t now = time(0);
        char* dt = ctime(&now);
        cout << "Finished update " << dt
        	<< iter_nedge << " edges, " << iter_nstate << " states, "
        	<< bad_update << " bad, " << early_update << " early" << endl;
        cout.flush();

        if (verbose >= 1)
        	cerr << "Start development parsing iter " << iter << endl;
        ThreadPool pool(config.GetInt("threads"), 1000);
        OutputCollector collector;
        vector<Parser::Result> results(dev_data_.size());
		for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
			if (!dev_data_[sent] || !dev_ranks_[sent])  // if leaving-one-out, it would be NULL
				continue;
			ShiftReduceTask *task = new ShiftReduceTask(sent, *dev_data_[sent],
					*dev_ranks_[sent], model_, features_, config, results[sent], collector);
			pool.Submit(task);
        }
		pool.Stop(true);

		// Calculate the losses
        vector<pair<double,double> > sums(losses_.size(), pair<double,double>(0,0));
        for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
        	if (!dev_data_[sent] || !dev_ranks_[sent])  // if leaving-one-out, it would be NULL
        		continue;
        	for(int i = 0; i < (int) losses_.size(); i++) {
        		pair<double,double> my_loss =
        				losses_[i]->CalculateSentenceLoss(results[sent].order,dev_ranks_[sent],NULL);
        		sums[i].first += my_loss.first;
        		sums[i].second += my_loss.second;
        		double acc = my_loss.second == 0 ?
        				1 : (1-my_loss.first/my_loss.second);
        		if (verbose >= 1){
        			if(i != 0) cerr << "\t";
        			cerr << losses_[i]->GetName() << "=" << acc
        					<< " (loss "<<my_loss.first<< "/" <<my_loss.second<<")";
        		}
        	}
        	if (verbose >= 1)
        		cerr << endl;
        }
		double prec = 0;
		for(int i = 0; i < (int) sums.size(); i++) {
			if(i != 0) cout << "\t";
			cout << losses_[i]->GetName() << "="
					<< (1 - sums[i].first/sums[i].second)
					<< " (loss "<<sums[i].first/losses_[i]->GetWeight() << "/"
					<<sums[i].second/losses_[i]->GetWeight()<<")";
			prec += (1 - sums[i].first/sums[i].second) * losses_[i]->GetWeight();
		}
		cout << endl;
        cout << "Finished iteration " << iter << ": " << prec << endl;
        cout.flush();
        // write every iteration to a different file
        if(config.GetBool("write_every_iter")){
        	stringstream ss;
        	ss << ".it" << iter;
        	WriteModel(config.GetString("model_out") + ss.str());
        }
        if(prec > best_prec) {
        	best_prec = prec;
        	best_iter = iter;
        	best_wlen = model_->GetWeights().size();
            WriteModel(config.GetString("model_out"));
            cout << "new high at iter " << iter << ": " << prec << endl;
        }
    }
    time_t now = time(0);
    char* dt = ctime(&now);
    cout << "Finished training " << dt
    	 << "peaked at iter " << best_iter << ": " << best_prec << ", |w|=" << best_wlen << endl;
}

void ShiftReduceTrainer::ReadData(const std::string & source_in,
		std::vector<Sentence*> & datas){
	std::ifstream in(source_in.c_str());
	if(!in) THROW_ERROR("Could not open source file: "
			<<source_in);
	std::string line;
	datas.clear();
	while(getline(in, line))
		datas.push_back(new Sentence(features_->ParseInput(line)));
}
void ShiftReduceTrainer::ReadAlignments(const std::string & align_in,
		std::vector<Ranks*> & ranks, std::vector<Sentence*> & datas) {
    std::ifstream in(align_in.c_str());
    if(!in) THROW_ERROR("Could not open alignment file: "
                            <<align_in);
    std::string line;
    int i = 0;
    while(getline(in, line))
        ranks.push_back(new
            Ranks(CombinedAlign((*SafeAccess(datas,i++))[0]->GetSequence(),
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