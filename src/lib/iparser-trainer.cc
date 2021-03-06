/*
 * shift-reduce-intermediate.cc
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */


#include "shift-reduce-dp/iparser-trainer.h"
#include "shift-reduce-dp/iparser-model.h"
#include "shift-reduce-dp/iparser-ranks.h"
#include "shift-reduce-dp/iparser.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <time.h>
#include <cstdlib>
using namespace std;
using namespace boost;

namespace lader {

void IParserTrainer::InitializeModel(const ConfigBase & config) {
	// TODO: restruture inheritance hierarchy
	ReordererEvaluator::InitializeModel(config);
    attach_trg_ = config.GetString("attach_trg") == "left" ?
                CombinedAlign::ATTACH_NULL_LEFT :
                CombinedAlign::ATTACH_NULL_RIGHT;
    srand(time(NULL));
    ofstream model_out(config.GetString("model_out").c_str());
    if(!model_out)
        THROW_ERROR("Must specify a valid model output with -model_out ('"
                        <<config.GetString("model_out")<<"')");
    // Load the model from  a file if it exists, otherwise note features
    if(config.GetString("model_in").length() != 0) {
        ifstream in(config.GetString("model_in").c_str());
        if(!in) THROW_ERROR("Couldn't read model from file (-model_in '"
                            << config.GetString("model_in") << "')");
        features_ = FeatureSet::FromStream(in);
        model_ = IParserModel::FromStream(in);
    } else {
        IParserModel * model = new IParserModel;
        model->SetMaxState(config.GetInt("max_state"));
        model->SetMaxIns(config.GetDouble("max_ins"));
        model->SetMaxDel(config.GetDouble("max_del"));
        model->SetUseReverse(config.GetBool("use_reverse"));
        model_ = model;
        features_ = new FeatureSet;
        features_->ParseConfiguration(config.GetString("feature_profile"));
    }
    // Load the other config
    model_->SetCost(config.GetDouble("cost"));
    vector<string> losses, first_last;
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

void IParserTrainer::GetReferenceSequences(const std::string & align_in,
		std::vector<ActionVector> & refseq, std::vector<Sentence*> & datas){
	std::ifstream in(align_in.c_str());
	if(!in) THROW_ERROR("Could not open alignment file: "
			<<align_in);
	std::string line;
	int i = 0;
	IParserModel * model = dynamic_cast<IParserModel*>(model_);
	while(getline(in, line)){
		const vector<string> & srcs = (*SafeAccess(datas,i++))[0]->GetSequence();
		CombinedAlign cal1(srcs, Alignment::FromString(line),
				CombinedAlign::LEAVE_NULL_AS_IS, combine_, bracket_); 	// for delete
		CombinedAlign cal2(srcs, Alignment::FromString(line),
				attach_, combine_, bracket_);
		IParserRanks ranks(cal2, attach_trg_);
		// enable insert/delete if allowed
		if (model->GetMaxDel() > 0){
			if (model->GetMaxIns() > 0)
				ranks.Insert(&cal1);
			refseq.push_back(ranks.GetReference(&cal1));
		}
		else{
			if (model->GetMaxIns() > 0)
				ranks.Insert(&cal2);
			refseq.push_back(ranks.GetReference(&cal2));
		}
	}

}

void IParserTrainer::TrainIncremental(const ConfigBase & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"), data_);
    if(config.GetString("align_in").length()){
    	ReadAlignments(config.GetString("align_in"), ranks_, data_);
    }
    if(config.GetString("source_dev").length() && config.GetString("align_dev").length()){
    	ReadData(config.GetString("source_dev"), dev_data_);
    	ReadAlignments(config.GetString("align_dev"), dev_ranks_, dev_data_);
    	GetReferenceSequences(config.GetString("align_dev"), dev_refseq_, dev_data_);
    }
    else{ // use train data as dev
		ReadData(config.GetString("source"), dev_data_);
		ReadAlignments(config.GetString("align"), dev_ranks_, dev_data_);
		GetReferenceSequences(config.GetString("align"), dev_refseq_, dev_data_);
	}
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
    IParserModel * model = dynamic_cast<IParserModel*>(model_);
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        // Shuffle
        if(config.GetBool("shuffle"))
        	random_shuffle(sent_order.begin(), sent_order.end());

        int done = 0;
        unsigned long iter_nedge = 0, iter_nstate = 0, iter_nuniq = 0, iter_step = 0, iter_maxstep = 0;
        int bad_update = 0, early_update = 0, prev_early = 0, skipped = 0;
        if (verbose >= 1)
        	cerr << "Start training parsing iter " << iter << endl;
        BOOST_FOREACH(int sent, sent_order) {
        	if(++done% 100 == 0) cerr << ".";
        	if(done % (100*10) == 0) cerr << done << endl;
        	if (verbose >= 1)
        		cerr << endl << "Sentence " << sent << endl;
        	// copy the original reference sequence because it will be resized if early update
			int n = (*data_[sent])[0]->GetNumWords();
			// Produce the parse
        	// max # insert/delete is propostional to the sentence length
			IParser p(model->GetMaxIns()*n, model->GetMaxDel()*n);
            p.SetBeamSize(config.GetInt("beam"));
            p.SetVerbose(verbose);
        	Parser::Result result;
        	clock_gettime(CLOCK_MONOTONIC, &tstart);
			p.Search(*model, *features_, *data_[sent],	// obligatory
					ranks_[sent]);	// optional
			p.Update(&result, &update);
			// do not need to set result
        	clock_gettime(CLOCK_MONOTONIC, &tend);
        	search.tv_sec += tend.tv_sec - tstart.tv_sec;
        	search.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			if (!p.GetBest() || !p.GetGoldBest(p.GetBest()->GetStep())){
				if (verbose >= 1)
					cerr << "Parser cannot produce any gold derivation" << endl;
				skipped++;
				continue;
			}
        	if (verbose >= 1){
        		cerr << "Result:";
				for (int step = 0 ; step < result.actions.size() ; step++)
					cerr << " " << (char) result.actions[step] << "_" << step+1;
				cerr << endl;
				DPState * best = p.GetBeamBest(result.step);
        		cerr << "Beam trace:" << endl;
        		best->PrintTrace(cerr);
        	}
        	if (result.step != result.actions.size())
        		THROW_ERROR("Result step " << result.step << " != action size " << result.actions.size() << endl);
        	if (result.step < p.GetBest()->GetStep())
        		early_update++;
        	DPState * gold = p.GetGoldBest(result.step);
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
        	clock_gettime(CLOCK_MONOTONIC, &tstart);
        	FeatureMapInt feat_map;
        	p.Simulate(*model, *features_, refseq, *data_[sent], firstdiff, feat_map, +1); // positive examples
        	p.Simulate(*model, *features_, result.actions, *data_[sent], firstdiff, feat_map, -1); // negative examples
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
        	iter_step += result.step;
        	iter_maxstep += p.GetBest()->GetStep();
        	iter_nedge += p.GetNumEdges();
        	iter_nstate += p.GetNumStates();
        	iter_nuniq += p.GetNumUniq();
        }
        cerr << "Running time on average: " << setprecision(4)
        		<< "searh " << ((double)search.tv_sec + 1.0e-9*search.tv_nsec) / (iter+1) << "s"
        		<< ", simulate " << ((double)simulate.tv_sec + 1.0e-9*simulate.tv_nsec) / (iter+1) << "s" << endl;
        time_t now = time(0);
        char* dt = ctime(&now);
        cout << "Finished update " << dt
			<< iter_nedge << " edges, " << iter_nstate << " states, " << iter_nuniq << " uniq, "
			<< setprecision(4)
        	<< skipped << " skip (" << 100.0*skipped/done << "%), "
        	<< bad_update << " bad (" << 100.0*bad_update/done << "%), "
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
			Task *task = new IParserTask(sent, *dev_data_[sent],
					model, features_, config,
					results[sent], collector, result_kbests[sent]);
			pool.Submit(task);
        }
		pool.Stop(true);

		// Calculate the losses
        vector<pair<double,double> > sum_losses(losses_.size(), pair<double,double>(0,0));
        vector<pair<double,double> > sum_losses_kbests(losses_.size(), pair<double,double>(0,0));
        for (int sent = 0 ; sent < dev_data_.size() ; sent++) {
        	ActionVector & refseq = dev_refseq_[sent];
			if (refseq.empty())
				continue;
			if (verbose >= 1){
				cerr << "Rank:";
				BOOST_FOREACH(int rank, dev_ranks_[sent]->GetRanks())
					cerr << " " << rank;
				cerr << endl;
				cerr << "Reference Action:";
				BOOST_FOREACH(DPState::Action action, refseq)
					cerr << " " << (char)action;
				cerr << endl;
				cerr << "Result ActionSeq:";
				BOOST_FOREACH(DPState::Action action, results[sent].actions)
					cerr << " " << (char)action;
				cerr << endl;
			}
			// For Levenshetein loss, do not use dev_rank_[sent]
			// We need to compare two sequences
			// Therefore, produce a parse using refseq,
			// and abuse Rank to set the result
			int n = (*dev_data_[sent])[0]->GetNumWords();
			IParser p(n, n);
			DPState * goal = p.GuidedSearch(refseq, n);
			if (!goal || !goal->Allow(DPState::IDLE, n)){
				if (verbose >= 1)
					cerr << "Parser cannot produce the goal state" << endl;
				continue;
			}
			Parser::Result result;
			Parser::SetResult(result, goal);
			if (verbose >= 1){
				cerr << "Oracle Purmutation:";
				BOOST_FOREACH(int order, result.order)
					cerr << " " << order;
				cerr << endl;
				cerr << "Result Purmutation:";
				BOOST_FOREACH(int order, results[sent].order)
					cerr << " " << order;
				cerr << endl;
			}
			Ranks ranks;
			ranks.SetRanks(result.order);
        	for(int i = 0; i < (int) losses_.size(); i++) {
				losses_[i]->Initialize(NULL, NULL);
        		pair<double,double> my_loss =
        				losses_[i]->CalculateSentenceLoss(results[sent].order, &ranks, NULL);
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
            				losses_[i]->CalculateSentenceLoss(result.order, &ranks, NULL);
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
			cout << " " << losses_[i]->GetName() << "=" << setprecision(3)
					<< (1 - sum_losses[i].first/sum_losses[i].second) << setprecision(6)
					<< " (loss "<<sum_losses[i].first/losses_[i]->GetWeight() << "/"
					<<sum_losses[i].second/losses_[i]->GetWeight()<<")";
			prec += (1 - sum_losses[i].first/sum_losses[i].second) * losses_[i]->GetWeight();
		}
		cout << endl;
		for(int i = 0; i < (int) sum_losses_kbests.size(); i++) {
			if(i != 0) cout << "\t";
			cout << "*" << losses_[i]->GetName() << "=" << setprecision(3)
					<< (1 - sum_losses_kbests[i].first/sum_losses_kbests[i].second) << setprecision(6)
					<< " (loss "<<sum_losses_kbests[i].first/losses_[i]->GetWeight() << "/"
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
