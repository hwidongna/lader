#include <lader/reorderer-trainer.h>
#include <boost/algorithm/string.hpp>
#include <lader/discontinuous-hyper-graph.h>
#include <lader/feature-data-sequence.h>
#include <boost/filesystem.hpp>
#include <time.h>
#include <cstdlib>
using namespace lader;
using namespace boost;
using namespace std;

namespace fs = ::boost::filesystem;

inline filesystem::path GetFeaturePath(const ConfigTrainer & config, int sent)
{
    fs::path dir(config.GetString("features_dir"));
    fs::path prefix(config.GetString("source_in"));
    fs::path file(prefix.filename().string() + "." + to_string(sent));
    fs::path full_path = dir / file;
    return full_path;
}

void ReordererTrainer::TrainIncremental(const ConfigTrainer & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"));
    if(config.GetString("align_in").length())
        ReadAlignments(config.GetString("align_in"));
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    int threads = config.GetInt("threads");
    int beam_size = config.GetInt("beam");
    int pop_limit = config.GetInt("pop_limit");
    int gapSize = config.GetInt("gap-size");
    int max_seq = config.GetInt("max-seq");
    bool full_fledged = config.GetBool("full_fledged");
    bool mp = config.GetBool("mp");
    bool cube_growing = config.GetBool("cube_growing");
    bool loss_aug = config.GetBool("loss_augmented_inference");
    int samples = config.GetInt("samples");
    // Temporary values
    double model_score = 0, model_loss = 0, oracle_score = 0, oracle_loss = 0;
    FeatureVectorInt model_features, oracle_features;
    std::tr1::unordered_map<int,double> feat_map;

    vector<int> sent_order(data_.size());

    // This is useful when you run trainer again with different parameters
    // for the same -source_in and -parse_in
    if (config.GetBool("reuse_features")){
        fs::directory_iterator it(config.GetString("features_dir"));
        fs::directory_iterator endit;
        fs::path stem(config.GetString("source_in"));
        stem = stem.filename();
        int i;
        for (i = 0 ; it != endit ; it++)
        	if (fs::is_regular_file(*it) && it->path().stem() == stem)
        		// sentence id is the extension of the file
        		// (+1 required as path().extension() starts with ".")
        		sent_order[i++] = atoi(it->path().extension().c_str()+1);
        // don't care the rest of sentences
        if (i < samples)
        	THROW_ERROR("if -reuse_features, set -samples " << samples
        			<< " <= stored sentences " << i)
        if (i > sent_order.size())
        	THROW_ERROR("-source_in has fewer sentences " << sent_order.size()
        			<< " than the sentences in -features_dir " << i)
        sent_order.resize(i);
        if(!config.GetBool("shuffle"))
        	sort(sent_order.begin(), sent_order.end());
    }
    else
    	for(int i = 0 ; i < (int)(sent_order.size());i++)
    		sent_order[i] = i;

    cerr << "Total " << sent_order.size() << " sentences, "
    		<< "Sample " << samples << " sentences" << endl;

    // TODO: parallize the feature extraction at sentence level
	if (config.GetBool("save_features"))
        saved_graphs_.resize((int)sent_order.size(), NULL);
    // Shuffle the whole orders for the first time
	if(config.GetBool("shuffle"))
		random_shuffle(sent_order.begin(), sent_order.end());
    cerr << "(\".\" == 100 sentences)" << endl;
	struct timespec build={0,0}, oracle={0,0}, model={0,0}, adjust={0,0};
	struct timespec tstart={0,0}, tend={0,0};
	DiscontinuousHyperGraph graph(gapSize, max_seq, cube_growing, full_fledged, mp, verbose);
	if (!config.GetString("bigram").empty())
		graph.LoadLM(config.GetString("bigram").c_str());
	graph.SetThreads(threads);
	graph.SetBeamSize(beam_size);
	graph.SetPopLimit(pop_limit);
	if (config.GetString("model_in").empty() && threads > 1)
		cerr << "-threads > 1 will be faster with -model_in" << endl;
	graph.SetSaveFeatures(config.GetBool("save_features"));
	HyperGraph * ptr_graph = &graph;
    // Perform an iteration
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        double iter_model_loss = 0, iter_oracle_loss = 0;
        int done = 0;
        // Shuffle the sample size
        if(iter != 0 && config.GetBool("shuffle"))
            random_shuffle(sent_order.begin(),
            		sent_order.size() > samples ? sent_order.begin() + samples: sent_order.end());
        // Over all values in the corpus
        BOOST_FOREACH(int sent, sent_order) {
        	if (done++ >= samples)
        		break;
        	if (verbose > 1)
        		cerr << "Sentence " << sent << endl;
            if(done % 100 == 0) cerr << ".";
            if(done % (100*10) == 0) cerr << done << endl;
            // If we are saving features for efficiency, recover the saved
            // features and replace them in the hypergraph
            if(config.GetBool("save_features")){
            	if (iter == 0) // nothing to recover for the first time
            		ptr_graph = graph.Clone();
            	else // reuse the saved graph
            		ptr_graph = saved_graphs_[sent];

            	if (iter == 0 || !config.GetString("features_dir").empty()){
            		ptr_graph->SetNumWords(data_[sent][0]->GetNumWords());
            		ptr_graph->InitStacks();
            	}
            	// Load the saved features in the file
            	// Reuse the saved features in the file from the first iteration
            	if((iter != 0 || config.GetBool("reuse_features"))
            	&& !config.GetString("features_dir").empty()){
            		clock_gettime(CLOCK_MONOTONIC, &tstart);
            		ifstream in(GetFeaturePath(config, sent).c_str());
            		ptr_graph->FeaturesFromStream(in);
            		in.close();
            		clock_gettime(CLOCK_MONOTONIC, &tend);
            		build.tv_sec += tend.tv_sec - tstart.tv_sec;
            		build.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
            		if(verbose > 0)
            			printf("FeaturesFromStream took about %.5f seconds\n",
            					((double) ((tend.tv_sec))+ 1.0e-9 * tend.tv_nsec)
            					- ((double) ((tstart.tv_sec)) + 1.0e-9 * tstart.tv_nsec));
            	}
            }

            clock_gettime(CLOCK_MONOTONIC, &tstart);
            // TODO: a loss-augmented parsing would result a different forest
            // (but it is impossible in decoding time)
            // We want to find a derivation that minimize loss and maximize model score
            // Make the hypergraph using cube pruning/growing
			ptr_graph->BuildHyperGraph(*model_,
                                        *features_,
                                        *non_local_features_,
                                        data_[sent]);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			build.tv_sec += tend.tv_sec - tstart.tv_sec;
			build.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			if (verbose > 0)
				printf(	"BuildHyperGraph took about %.5f seconds\n",
						((double) (tend.tv_sec) + 1.0e-9 * tend.tv_nsec)
						- ((double) (tstart.tv_sec)+ 1.0e-9 * tstart.tv_nsec));
			if (!ptr_graph->GetBest()){
				cerr << "Fail to produce a tree for sentence " << sent << endl;
				continue;
			}
			// Add losses to the hypotheses in the hypergraph
			BOOST_FOREACH(LossBase * loss, losses_)
						ptr_graph->AddLoss(loss,
							sent < (int)ranks_.size() ? &ranks_[sent] : NULL,
							sent < (int)parses_.size() ? &parses_[sent] : NULL);
			// Parse the hypergraph, penalizing loss heavily (oracle)
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			oracle_score = ptr_graph->Rescore(-1e6);
			feat_map.clear();
			ptr_graph->AccumulateFeatures(feat_map, *model_, *features_,
							*non_local_features_, data_[sent],
							ptr_graph->GetBest());
			oracle_features.clear();
			ClearAndSet(oracle_features, feat_map);
			oracle_loss = ptr_graph->GetBest()->AccumulateLoss();
			oracle_score -= oracle_loss * -1e6;
			clock_gettime(CLOCK_MONOTONIC, &tend);
			oracle.tv_sec += tend.tv_sec - tstart.tv_sec;
			oracle.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			// Parse the hypergraph, slightly boosting loss by 1.0 if we are
			// using loss-augmented inference
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			model_score = ptr_graph->Rescore(loss_aug ? 1.0 : 0.0);
			model_loss = ptr_graph->GetBest()->AccumulateLoss();
			model_score -= model_loss * 1;
			feat_map.clear();
			ptr_graph->AccumulateFeatures(feat_map, *model_, *features_,
							*non_local_features_, data_[sent],
							ptr_graph->GetBest());
			ClearAndSet(model_features, feat_map);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			model.tv_sec += tend.tv_sec - tstart.tv_sec;
			model.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			// Add the statistics for this iteration
			iter_model_loss += model_loss;
			iter_oracle_loss += oracle_loss;
			if (verbose > 0){
				vector<int> order;
				ptr_graph->GetBest()->GetReordering(order, verbose > 1);
				for(int i = 0; i < (int)order.size(); i++) {
					if(i != 0) cout << " "; cout << order[i];
				}
				cout << endl;
				for(int i = 0; i < (int)order.size(); i++) {
					if(i != 0) cout << " "; cout << data_[sent][0]->GetElement(order[i]);
				}
				cout << endl;
				for(int i = 0; i < (int)order.size(); i++) {
					if(i != 0) cout << " "; cout << ranks_[sent][i];
				}
				cout << endl;
				// --- DEBUG: check both losses match ---
				double sent_loss = 0;
				BOOST_FOREACH(LossBase * loss, losses_)
					sent_loss += loss->CalculateSentenceLoss(order,
									sent < (int)ranks_.size() ? &ranks_[sent] : NULL,
									sent < (int)parses_.size() ? &parses_[sent] : NULL).first;
				if(sent_loss != model_loss){
					ostringstream out;
					vector<string> words = ((FeatureDataSequence*)data_[sent][0])->GetSequence();
		            ptr_graph->GetBest()->PrintParse(words, out);
					cerr << "sent=" << sent << " sent_loss="<< sent_loss <<" != model_loss="<<model_loss << endl;
		            cerr << out.str() << endl;
				}
				cout << "sent=" << sent << " oracle_score=" << oracle_score << " model_score=" << model_score << endl
					 << "sent_loss = "<< sent_loss << " oracle_loss=" << oracle_loss << " model_loss=" << model_loss << endl;
			}
			// Add the difference between the vectors if there is at least
			//  some loss
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			if(config.GetString("learner") == "pegasos") {
				model_->AdjustWeightsPegasos(
						model_loss == oracle_loss ?
								FeatureVectorInt() :
								VectorSubtract(oracle_features, model_features));
			} else if(config.GetString("learner") == "perceptron") {
				if(model_loss != oracle_loss)
					model_->AdjustWeightsPerceptron(
							VectorSubtract(oracle_features, model_features));
			} else {
				THROW_ERROR("Bad learner: " << config.GetString("learner"));
			}

			clock_gettime(CLOCK_MONOTONIC, &tend);
			adjust.tv_sec += tend.tv_sec - tstart.tv_sec;
			adjust.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
			// If we are saving features
			if(config.GetBool("save_features") && iter == 0){
                if (saved_graphs_[sent])
                    THROW_ERROR("Graph is already stored for sentence " << sent)
				saved_graphs_[sent] = ptr_graph;
			}
			// because features are stored in stack, clear stack if not saved in memory
			if(!config.GetBool("save_features") || !config.GetString("features_dir").empty()){
				// if -reuse_features, do not rewrite the features
				if (iter == 0 && !config.GetBool("reuse_features")){
					ofstream out(GetFeaturePath(config, sent).c_str());
					ptr_graph->FeaturesToStream(out);
					out.close();
				}
				// clear stack every iteration
            	ptr_graph->ClearStacks();
			}
        }
        cerr << "Finished iteration " << iter << endl;
        cout << "Finished iteration " << iter << " with loss " << iter_model_loss << " (oracle: " << iter_oracle_loss << ")" << endl;
        cout << "Running time on average: "
        		<< "build " << ((double)build.tv_sec + 1.0e-9*build.tv_nsec) / (iter+1) << "s"
        		<< ", oracle " << ((double)oracle.tv_sec + 1.0e-9*oracle.tv_nsec) / (iter+1) << "s"
        		<< ", model " << ((double)model.tv_sec + 1.0e-9*model.tv_nsec)  / (iter+1) << "s"
        		<< ", adjust " << ((double)adjust.tv_sec + 1.0e-9*adjust.tv_nsec)  / (iter+1) << "s" << endl;
        cout.flush();
        bool last_iter = (iter_model_loss == iter_oracle_loss ||
                          iter == config.GetInt("iterations") - 1);
        // write every iteration to a different file
        if(config.GetBool("write_every_iter")){
        	stringstream ss;
        	ss << ".it" << iter;
        	WriteModel(config.GetString("model_out") + ss.str(), config.GetBool("backward_compatibility"));
        }
        if(last_iter) {
            WriteModel(config.GetString("model_out"), config.GetBool("backward_compatibility"));
            break;
        }
    }
}

void ReordererTrainer::InitializeModel(const ConfigTrainer & config) {
    srand(time(NULL));
    // Load the model from  a file if it exists, otherwise note features

    if (config.GetBool("backward_compatibility") && config.GetBool("reuse_features"))
    	THROW_ERROR("-backward_compatibility and -reuse_features cannot be used together.")
	if (config.GetBool("reuse_features")
	&& (config.GetString("model_in").empty() || config.GetString("features_dir").empty()))
		THROW_ERROR("-reuse_features requires -model_in and -features_dir")
    if(!config.GetString("model_in").empty()) {
        std::ifstream in(config.GetString("model_in").c_str());
        if(!in) THROW_ERROR("Couldn't read model from file (-model_in '"
                            << config.GetString("model_in") << "')");
        cerr << "Load the existing model" << endl;
        features_ = FeatureSet::FromStream(in);
        if (!config.GetBool("backward_compatibility"))
        	non_local_features_ = FeatureSet::FromStream(in);
        if (config.GetBool("backward_compatibility"))
        	model_ = ReordererModel::FromStreamOld(in);
        else
        	model_ = ReordererModel::FromStream(in);
    }
    else {
        model_ = new ReordererModel;
        features_ = new FeatureSet;
        non_local_features_ = new FeatureSet;
        features_->ParseConfiguration(config.GetString("feature_profile"));
        non_local_features_->ParseConfiguration(config.GetString("non_local_feature_profile"));
        features_->SetMaxTerm(config.GetInt("max_term"));
        features_->SetUseReverse(config.GetBool("use_reverse"));
    }
    ofstream model_out(config.GetString("model_out").c_str());
    if(!model_out)
        THROW_ERROR("Must specify a valid model output with -model_out ('"
                        <<config.GetString("model_out")<<"')");
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

void ReordererTrainer::ReadAlignments(const std::string & align_in) {
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

void ReordererTrainer::ReadParses(const std::string & parse_in) {
    std::ifstream in(parse_in.c_str());
    if(!in) THROW_ERROR("Could not open parse file (-parse_in): " << parse_in);
    std::string line;
    while(getline(in, line)) {
        parses_.push_back(FeatureDataParse());
        parses_.rbegin()->FromString(line);
    }
}
