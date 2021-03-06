/*
 * shift-reduce-trainer.cc
 *
 *  Created on: Dec 24, 2013
 *      Author: leona
 */

#include "shift-reduce-dp/shift-reduce-trainer.h"
#include "shift-reduce-dp/shift-reduce-model.h"
#include "shift-reduce-dp/parser.h"
#include "lader/feature-data-sequence.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>
#include <cstdlib>
using namespace std;
using namespace boost;

namespace lader {

void ShiftReduceTrainer::InitializeModel(const ConfigBase & config) {
	ReordererEvaluator::InitializeModel(config);
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
        model_ = ShiftReduceModel::FromStream(in);
    } else {
        ShiftReduceModel * model = new ShiftReduceModel;
        model->SetMaxTerm(config.GetInt("max_term"));
        model->SetMaxState(config.GetInt("max_state"));
        model->SetMaxSwap(config.GetInt("max_swap"));
        model->SetUseReverse(config.GetBool("use_reverse"));
        model_ = model;
        features_ = new FeatureSet;
        features_->ParseConfiguration(config.GetString("feature_profile"));
    }
    // Load the other config
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

void ShiftReduceTrainer::TrainIncremental(const ConfigBase & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"), data_);
    if(config.GetString("align_in").length()){
        ReadAlignments(config.GetString("align_in"), ranks_, data_);
    }
    if(config.GetString("source_dev").length() && config.GetString("align_dev").length()){
    	ReadData(config.GetString("source_dev"), dev_data_);
    	ReadAlignments(config.GetString("align_dev"), dev_ranks_, dev_data_);
    }
    else{ // use train data as dev
    	ReadData(config.GetString("source_in"), dev_data_);
		ReadAlignments(config.GetString("align_in"), dev_ranks_, dev_data_);
    }
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    int m = config.GetInt("max_swap");
    bool loss_aug = config.GetBool("loss_augmented_inference");
    vector<int> sent_order(data_.size());
	for(int i = 0 ; i < (int)sent_order.size(); i++)
		sent_order[i] = i;

    cerr << "(\".\" == 100 sentences)" << endl;
    string update = config.GetString("update");
    struct timespec search={0,0}, simulate={0,0};
    struct timespec tstart={0,0}, tend={0,0};
    double model_score = 0, model_loss = 0, oracle_score = 0, oracle_loss = 0;
    FeatureVectorInt model_features, oracle_features;
	FeatureMapInt feat_map;
    double best_prec = 0;
    int best_iter, best_wlen;
    ShiftReduceModel * model = dynamic_cast<ShiftReduceModel*>(model_);
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        // Shuffle
        if(config.GetBool("shuffle"))
        	random_shuffle(sent_order.begin(), sent_order.end());

        int done = 0;
        double iter_model_loss = 0, iter_oracle_loss = 0, iter_total_loss = 0;
        unsigned long iter_nedge = 0, iter_nstate = 0, iter_nuniq = 0, iter_ngold = 0, iter_step = 0, iter_maxstep = 0;
        int bad_update = 0, early_update = 0, prev_early = 0, skipped = sent_order.size();
        if (verbose >= 1)
        	cerr << "Start training parsing iter " << iter << endl;
        BOOST_FOREACH(int sent, sent_order) {
        	if(++done% 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;

        	if (verbose >= 1){
        		cerr << endl << "Sentence " << sent << endl;
        	}
        	if (verbose >= 1){
        		cerr << "Rank:";
        		BOOST_FOREACH(int rank, ranks_[sent]->GetRanks())
        			cerr << " " << rank;
        		cerr << endl;
        	}
        	int n = (*data_[sent])[0]->GetNumWords();
            Parser * p;
            if (m > 0)
            	p = new DParser(m);
            else
            	p = new Parser();
            p->SetBeamSize(config.GetInt("beam"));
            p->SetVerbose(verbose);
			Parser::Result result;
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			p->Search(*model, *features_, *data_[sent],	// obligatory
						ranks_[sent]);	// optional
//			// do not need to set result
//			p->Search(*model, *features_, *data_[sent]);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			search.tv_sec += tend.tv_sec - tstart.tv_sec;
			search.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			// try update against gold
			if (update != "none" && p->GetGoldBest(p->GetBest()->GetStep())){
			// TODO: temporary for experiment (2014/09/10)
//			if (m == 0 && p->GetGoldBest(p->GetBest()->GetStep())){
				p->Update(&result, &update);
				if (verbose >= 1){
					cerr << "Result:";
					for (int step = 0 ; step < result.actions.size() ; step++)
						cerr << " " << (char) result.actions[step] << "_" << step+1;
					cerr << endl;
					DPState * best = p->GetBeamBest(result.step);
					cerr << "Beam trace:" << endl;
					best->PrintTrace(cerr);
				}
				if (result.step != result.actions.size())
					THROW_ERROR("Result step " << result.step << " != action size " << result.actions.size() << endl);
				if (result.step < p->GetBest()->GetStep())
					early_update++;
				DPState * gold = p->GetGoldBest(result.step);
				if (!gold)
					THROW_ERROR("Fail to get the gold derivation at step " << result.step << endl);
				ActionVector refseq;
				gold->AllActions(refseq);
				if (verbose >= 1){
					cerr << "Gold:  ";
					for (int step = 0 ; step < refseq.size() ; step++)
						cerr << " " << (char) refseq[step] << "_" << step+1;
					cerr << endl;
					cerr << "Beam trace:" << endl;
					gold->PrintTrace(cerr);
				}
				int firstdiff;
				for (firstdiff = 1 ; firstdiff <= result.step ; firstdiff++)
					if (result.actions[firstdiff-1] != refseq[firstdiff-1])
						break;
				if (verbose >= 1 && result.step >= firstdiff)
					cerr << "Update from " << firstdiff << endl;
				FeatureMapInt feat_map;
				p->Simulate(*model, *features_, refseq, *data_[sent], firstdiff, feat_map, +1); // positive examples
				p->Simulate(*model, *features_, result.actions, *data_[sent], firstdiff, feat_map, -1); // negative examples
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
				skipped--;
			}
			// update against the min loss (oracle) and max score (model)
			else if (config.GetString("learner") != "none"){
				Parser::SetResult(result, p->GetBest());
				BOOST_FOREACH(LossBase * loss, losses_)
					p->AddLoss(loss,
						sent < (int)ranks_.size() ? ranks_[sent] : NULL,
						sent < (int)parses_.size() ? &parses_[sent] : NULL);
				// penalizing loss heavily (oracle)
				oracle_score = p->Rescore(-1e6);
				oracle_loss = p->AccumulateLoss(p->GetBest());
				oracle_score -= oracle_loss * -1e6;
				feat_map.clear();
				if (verbose >= 2){
					p->GetBest()->Print(cerr);
					cerr << " score: " << oracle_score << " loss: " << oracle_loss << endl;
				}
				p->AccumulateFeatures(feat_map, *model_, *features_,
								(*data_[sent]), p->GetBest());
				oracle_features.clear();
				ClearAndSet(oracle_features, feat_map);
				// slightly boosting loss by 1.0 if we are using loss-augmented inference
				model_score = p->Rescore(loss_aug ? 1.0 : 0.0);
				model_loss = p->AccumulateLoss(p->GetBest());
				model_score -= model_loss * 1;
				feat_map.clear();
				if (verbose >= 2){
					p->GetBest()->Print(cerr);
					cerr << " score: " << model_score << " loss: " << model_loss << endl;
				}
				p->AccumulateFeatures(feat_map, *model_, *features_,
								(*data_[sent]), p->GetBest());
				ClearAndSet(model_features, feat_map);
				// Add the statistics for this iteration
				iter_model_loss += model_loss;
				iter_oracle_loss += oracle_loss;
				vector<int> order;
				p->GetBest()->GetReordering(order);
				// --- DEBUG: check both losses match ---
				double sent_loss = 0, sent_score = 0;
				BOOST_FOREACH(LossBase * loss, losses_){
					pair<double,double> l = loss->CalculateSentenceLoss(order,
							sent < (int)ranks_.size() ? ranks_[sent] : NULL,
									sent < (int)parses_.size() ? &parses_[sent] : NULL);
					sent_loss += l.first;
					iter_total_loss += l.second;
				}
				if(sent_loss != model_loss){
					ostringstream out;
					vector<string> words = ((FeatureDataSequence*)(*data_[sent])[0])->GetSequence();
					p->GetBest()->PrintParse(words, out);
					cerr << "sent=" << sent << " sent_loss="<< sent_loss <<" != model_loss="<< model_loss << endl;
					cerr << out.str() << endl;
				}
				FeatureVectorInt deltafeats = VectorSubtract(oracle_features, model_features);
				if (model_loss != oracle_loss){
					if (model_->ScoreFeatureVector(deltafeats) > 0){
						bad_update++;
						if (verbose >= 1)
							cerr << "Bad update at Sentence " << sent << endl;
					}
					skipped--;
				}
				if(config.GetString("learner") == "pegasos") {
					model_->AdjustWeightsPegasos(
							model_loss == oracle_loss ?
									FeatureVectorInt() : deltafeats);
				} else if(config.GetString("learner") == "perceptron") {
					if(model_loss != oracle_loss)
						model_->AdjustWeightsPerceptron(deltafeats);
				} else {
					THROW_ERROR("Bad learner: " << config.GetString("learner"));
				}
			}
			clock_gettime(CLOCK_MONOTONIC, &tend);
			simulate.tv_sec += tend.tv_sec - tstart.tv_sec;
			simulate.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			iter_step += result.step;
			iter_maxstep += p->GetBest()->GetStep();
			iter_nedge += p->GetNumEdges();
			iter_nstate += p->GetNumStates();
			iter_nuniq += p->GetNumUniq();
			delete p;
        }
        // output model/oracel losses for each iteration
        cout << " ";
        for (int i = 0 ; i < losses_.size() ; i++){
        	if (i != 0) cout << " + ";
        	cout << losses_[i]->GetName() << "*" << losses_[i]->GetWeight();
        }
        cout << "=" << 1 - iter_model_loss/iter_total_loss
        	 << " (loss " << iter_model_loss << "/" << iter_total_loss << ")" << endl;
        cout << "*";
        for (int i = 0 ; i < losses_.size() ; i++){
			if (i != 0) cout << " + ";
			cout << losses_[i]->GetName() << "*" << losses_[i]->GetWeight();
		}
        cout << "=" << 1 - iter_oracle_loss/iter_total_loss <<
        		" (loss " << iter_oracle_loss << "/" << iter_total_loss << ")" << endl;
        cerr << "Running time on average: " << std::setprecision(4)
        		<< "searh " << ((double)search.tv_sec + 1.0e-9*search.tv_nsec) / (iter+1) << "s"
        		<< ", simulate " << ((double)simulate.tv_sec + 1.0e-9*simulate.tv_nsec) / (iter+1) << "s" << endl;
		time_t now = time(0);
		char* dt = ctime(&now);
		cout << "Finished update " << dt
			<< iter_nedge << " edges, " << iter_nstate << " states, " << iter_nuniq << " uniq, "
			<< std::setprecision(4)
			<< bad_update << " bad (" << 100.0*bad_update/done << "%), "
			<< skipped << " skip (" << 100.0*skipped/done << "%), "
			<< early_update << " early (" << 100.0*early_update/done << "%), "
			<< iter_step << "/" << iter_maxstep << " steps (" << 100.0*iter_step/iter_maxstep << "%)" << endl;
        cout.flush();
        if ((update == "early" || update == "max") && (early_update == 0 || prev_early == early_update)){
        	cout << "No more update" << endl;
        	break;
        }
        prev_early = early_update;
        if (verbose >= 1)
        	cerr << "Start development parsing iter " << iter << endl;
        ThreadPool pool(config.GetInt("threads"), 1000);
        OutputCollector collector;
        vector<Parser::Result> results(dev_data_.size());
        vector< vector<Parser::Result> > result_kbests(dev_data_.size());
		for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
			Task *task = new ShiftReduceTask(sent, *dev_data_[sent],
					model, features_, config,
					results[sent], collector, result_kbests[sent]);
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
			if (verbose >= 1){
				cerr << "Rank:";
				BOOST_FOREACH(int rank, dev_ranks_[sent]->GetRanks())
					cerr << " " << rank;
				cerr << endl;
				cerr << "Result ActionSeq:";
				BOOST_FOREACH(DPState::Action action, results[sent].actions)
					cerr << " " << (char)action;
				cerr << endl;
			}
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
        // Output the losses
		double prec = 0;
		for(int i = 0; i < (int) sum_losses.size(); i++) {
			if(i != 0) cout << "\t";
			cout << " " << losses_[i]->GetName() << "=" << std::setprecision(3)
					<< (1 - sum_losses[i].first/sum_losses[i].second);
			cout << std::setprecision(5)
					<< " (loss "<<sum_losses[i].first/losses_[i]->GetWeight() << "/"
					<< sum_losses[i].second/losses_[i]->GetWeight()<<")";
			prec += (1 - sum_losses[i].first/sum_losses[i].second) * losses_[i]->GetWeight();
		}
		cout << endl;
		for(int i = 0; i < (int) sum_losses_kbests.size(); i++) {
			if(i != 0) cout << "\t";
			cout << "*" << losses_[i]->GetName() << "=" << std::setprecision(3)
					<< (1 - sum_losses_kbests[i].first/sum_losses_kbests[i].second);
			cout << std::setprecision(5)
					<< " (loss "<<sum_losses_kbests[i].first/losses_[i]->GetWeight() << "/"
					<< sum_losses_kbests[i].second/losses_[i]->GetWeight()<<")";
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
        if (prec > 1 - MINIMUM_WEIGHT){
        	cout << "Reach the highest precision" << endl;
        	break;
        }
    }
    time_t now = time(0);
    char* dt = ctime(&now);
    cout << "Finished training " << dt
    	 << "peaked at iter " << best_iter << ": " << best_prec << ", |w|=" << best_wlen << endl;
}

} /* namespace lader */
