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

template <class T>
void Merge(vector<T> & to, vector<T> & from){
//	if (to.size() != from.size())
//		THROW_ERROR("vector size " << to.size() << " != " << from.size() << endl);
	for (int i = 0 ; i < to.size() ; i++)
		if (!to[i] && from[i]){
			to[i] = from[i];
			from[i] = NULL;
		}
}

void ShiftReduceTrainer::TrainIncremental(const ConfigBase & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"), data_);
    if(config.GetString("align_in").length())
        ReadAlignments(config.GetString("align_in"), ranks_, data_);
    if(config.GetString("source_dev").length() && config.GetString("align_dev").length()){
    	ReadData(config.GetString("source_dev"), dev_data_);
    	ReadAlignments(config.GetString("align_dev"), dev_ranks_, dev_data_);
    }
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    vector<int> sent_order(data_.size());
	for(int i = 0 ; i < (int)sent_order.size(); i++)
		sent_order[i] = i;

    cerr << "(\".\" == 100 sentences)" << endl;
    string update = config.GetString("update");
    struct timespec search={0,0}, simulate={0,0};
    struct timespec tstart={0,0}, tend={0,0};
    double best_prec = 0;
    int best_iter, best_wlen;
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        // Shuffle
        if(config.GetBool("shuffle"))
        	random_shuffle(sent_order.begin(), sent_order.end());
        // Split train and dev set for each iteration
        if(!(config.GetString("source_dev").length() && config.GetString("align_dev").length())){
        	int count1, count2;
        	do{
        		Merge(data_, dev_data_);
        		Merge(ranks_, dev_ranks_);
        		count1 = MoveRandom(data_, dev_data_, config.GetDouble("ratio_dev"));
        		count2 = MoveRandom(ranks_, dev_ranks_, config.GetDouble("ratio_dev"));
        	} while(count1 != count2 || count1 == data_.size());
			cerr << "Split " << data_.size() << " train set into "
					<< data_.size()-count1 << " train and " << count1 << " dev set" << endl;
        }

        int done = 0;
        double iter_nedge = 0, iter_nstate = 0;
        int bad_update = 0, early_update = 0;
        if (verbose >= 1)
        	cerr << "Start training parsing iter " << iter << endl;
        BOOST_FOREACH(int sent, sent_order) {
        	if (!ranks_[sent]) // it would be NULL
        		continue;
        	if(++done% 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;

        	if (verbose >= 1){
        		cerr << endl << "Sentence " << sent << endl;
        		cerr << "Rank:";
        		BOOST_FOREACH(int rank, ranks_[sent]->GetRanks())
        			cerr << " " << rank;
        		cerr << endl;
        	}
        	vector<DPState::Action> refseq = ranks_[sent]->GetReference();
        	if (verbose >= 1){
        		cerr << "Reference:";
        		BOOST_FOREACH(DPState::Action action, refseq)
        			cerr << " " << action;
        		cerr << endl;
        	}
        	if (refseq.size() < (*data_[sent])[0]->GetNumWords()*2 - 1){
        		if (verbose >= 1)
        			cerr << "Fail to get correct reference sequence, skip it" << endl;
        		continue;
        	}

            Parser p;
            p.SetBeamSize(config.GetInt("beam"));
            p.SetVerbose(verbose);
        	Parser::Result result;
        	clock_gettime(CLOCK_MONOTONIC, &tstart);
        	p.Search(*model_, *features_, *data_[sent], result, config.GetInt("max_state"),
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
        	p.Simulate(*model_, *features_, refseq, *data_[sent], step, feat_map, +1); // positive examples
        	p.Simulate(*model_, *features_, result.actions, *data_[sent], step, feat_map, -1); // negative examples
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
        vector< vector<Parser::Result> > result_kbests(dev_data_.size());
		for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
			if (!dev_data_[sent] || !dev_ranks_[sent])  // it would be NULL
				continue;
			ShiftReduceTask *task = new ShiftReduceTask(sent, *dev_data_[sent],
					*dev_ranks_[sent], model_, features_, config, results[sent], collector, result_kbests[sent]);
			pool.Submit(task);
        }
		pool.Stop(true);

		// Calculate the losses
        vector<pair<double,double> > sum_losses(losses_.size(), pair<double,double>(0,0));
        vector<pair<double,double> > sum_losses_kbests(losses_.size(), pair<double,double>(0,0));
        done = 0;
        for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
        	if(++done% 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;
        	if (!dev_data_[sent] || !dev_ranks_[sent])  // it would be NULL
        		continue;
        	for(int i = 0; i < (int) losses_.size(); i++) {
        		pair<double,double> my_loss =
        				losses_[i]->CalculateSentenceLoss(results[sent].order,dev_ranks_[sent],NULL);
        		sum_losses[i].first += my_loss.first;
        		sum_losses[i].second += my_loss.second;
        		double acc = my_loss.second == 0 ?
        				1 : (1-my_loss.first/my_loss.second);
        		if (verbose >= 1){
        			if(i != 0) cerr << "\t";
        			cerr << losses_[i]->GetName() << "=" << acc
        					<< " (loss "<<my_loss.first<< "/" <<my_loss.second<<")";
        		}
        		BOOST_FOREACH(Parser::Result & result, result_kbests[sent]){
            		pair<double,double> loss_k =
            				losses_[i]->CalculateSentenceLoss(result.order,dev_ranks_[sent],NULL);
            		double acc_k = loss_k.second == 0 ?
            				1 : (1-loss_k.first/loss_k.second);
            		if (acc_k > acc){
            			acc = acc_k;
            			my_loss = loss_k;
            		}
        		}
        		sum_losses_kbests[i].first += my_loss.first;
        		sum_losses_kbests[i].second += my_loss.second;
        	}
        	if (verbose >= 1)
        		cerr << endl;
        }
		double prec = 0;
		for(int i = 0; i < (int) sum_losses.size(); i++) {
			if(i != 0) cout << "\t";
			cout << losses_[i]->GetName() << "="
					<< (1 - sum_losses[i].first/sum_losses[i].second)
					<< " (loss "<<sum_losses[i].first/losses_[i]->GetWeight() << "/"
					<<sum_losses[i].second/losses_[i]->GetWeight()<<")";
			prec += (1 - sum_losses[i].first/sum_losses[i].second) * losses_[i]->GetWeight();
		}
		cout << endl;
		for(int i = 0; i < (int) sum_losses_kbests.size(); i++) {
			if(i != 0) cout << "\t";
			cout << losses_[i]->GetName() << "="
					<< (1 - sum_losses_kbests[i].first/sum_losses_kbests[i].second)
					<< " (kbest "<<sum_losses_kbests[i].first/losses_[i]->GetWeight() << "/"
					<<sum_losses_kbests[i].second/losses_[i]->GetWeight()<<")";
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

} /* namespace lader */
