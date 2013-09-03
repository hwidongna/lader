#include <lader/reorderer-trainer.h>
#include <boost/algorithm/string.hpp>
#include <lader/discontinuous-hyper-graph.h>

using namespace lader;
using namespace boost;
using namespace std;

void ReordererTrainer::TrainIncremental(const ConfigTrainer & config) {
    InitializeModel(config);
    ReadData(config.GetString("source_in"));
    if(config.GetString("align_in").length())
        ReadAlignments(config.GetString("align_in"));
    if(config.GetString("parse_in").length())
        ReadParses(config.GetString("parse_in"));
    int verbose = config.GetInt("verbose");
    int gapSize = config.GetInt("gap-size");
    bool full_fledged = config.GetBool("full_fledged");
    bool mp = config.GetBool("mp");
    bool cube_growing = config.GetBool("cube_growing");
    // Temporary values
    bool loss_aug = config.GetBool("loss_augmented_inference");
    double model_score = 0, model_loss = 0, oracle_score = 0, oracle_loss = 0;
    FeatureVectorInt model_features, oracle_features;
    std::tr1::unordered_map<int,double> feat_map;
    vector<int> sent_order(data_.size());
    for(int i = 0 ; i < (int)(sent_order.size());i++)
        sent_order[i] = i;

    if(gapSize > 0)
        cerr << "use a discontinuous hyper-graph: D=" << gapSize << endl;

    if(mp)
        cerr << "enable monotone at punctuation to prevent excessive reordering" << endl;
    // Perform an iteration
    cerr << "(\".\" == 100 sentences)" << endl;
    for(int iter = 0; iter < config.GetInt("iterations"); iter++) {
        double iter_model_loss = 0, iter_oracle_loss = 0;
        int done = 0;
        // Over all values in the corpus
        // Shuffle
        if(config.GetBool("shuffle"))
            random_shuffle(sent_order.begin(), sent_order.end());
        BOOST_FOREACH(int sent, sent_order) {
//        	int sent = sent_order[sample];
        	if (done >= config.GetInt("samples")){
        		break;
        	}
        	if (verbose > 1)
        		cerr << "Sentence " << sent << endl;
            if(++done % 100 == 0) { cout << "."; cout.flush(); }
            HyperGraph * hyper_graph = new DiscontinuousHyperGraph(gapSize, cube_growing, mp, full_fledged, verbose);
            // If we are saving features for efficiency, recover the saved
            // features and replace them in the hypergraph
            if(config.GetBool("save_features") && iter != 0)
                hyper_graph->SetFeatures(SafeAccess(saved_feats_, sent));
            // TODO: a loss-augmented parsing would result a different forest
            // Make the hypergraph using cube pruning/growing
			hyper_graph->BuildHyperGraph(model_,
                                        features_,
                                        non_local_features_,
                                        data_[sent],
                                        config.GetInt("beam"));
            // Add losses to the hypotheses in the hypergraph
            BOOST_FOREACH(LossBase * loss, losses_)
            	hyper_graph->AddLoss(loss,
					sent < (int)ranks_.size() ? &ranks_[sent] : NULL,
					sent < (int)parses_.size() ? &parses_[sent] : NULL);
            // Parse the hypergraph, penalizing loss heavily (oracle)
            oracle_score = hyper_graph->Rescore(-1e6);
			feat_map.clear();
			hyper_graph->AccumulateNonLocalFeatures(feat_map,
					model_,
					non_local_features_,
					data_[sent],
					*hyper_graph->GetBest());
			oracle_features = hyper_graph->GetBest()->AccumulateFeatures(feat_map, hyper_graph->GetFeatures());
			oracle_loss = hyper_graph->GetBest()->AccumulateLoss();
			oracle_score -= oracle_loss * -1e6;
			// Parse the hypergraph, slightly boosting loss by 1.0 if we are
			// using loss-augmented inference
			model_score = hyper_graph->Rescore(loss_aug ? 1.0 : 0.0);
			model_loss = hyper_graph->GetBest()->AccumulateLoss();
			model_score -= model_loss * 1;
			// Add the statistics for this iteration
			iter_model_loss += model_loss;
			iter_oracle_loss += oracle_loss;
			if (verbose > 0){
				vector<int> order;
//				hyper_graph->GetRoot()->GetReordering(order);
				hyper_graph->GetReordering(order, hyper_graph->GetBest());
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
				if(sent_loss != model_loss)
					THROW_ERROR("sent_loss="<<sent_loss
							<<", model_loss="<<model_loss);
				cout << "sent=" << sent << " oracle_score=" << oracle_score << " model_score=" << model_score << endl
					 << "sent_loss = "<< sent_loss << " oracle_loss=" << oracle_loss << " model_loss=" << model_loss << endl;
			}
			feat_map.clear();
			hyper_graph->AccumulateNonLocalFeatures(feat_map,
							model_,
							non_local_features_,
							data_[sent],
							*hyper_graph->GetBest());
			model_features = hyper_graph->GetBest()->AccumulateFeatures(feat_map, hyper_graph->GetFeatures());
			// Add the difference between the vectors if there is at least
			//  some loss
			model_.AdjustWeights(model_loss == oracle_loss ? FeatureVectorInt() : VectorSubtract(oracle_features, model_features));
			// If we are saving features
			if(config.GetBool("save_features")){
				if((int)((saved_feats_.size())) <= sent)
                    saved_feats_.resize(sent+1);
                saved_feats_[sent] = hyper_graph->GetFeatures();
                hyper_graph->ClearFeatures();
            }
            delete hyper_graph;
        }
        cout << "Finished iteration " << iter << " with loss " << iter_model_loss << " (oracle: " << iter_oracle_loss << ")" << endl;
        bool last_iter = (iter_model_loss == iter_oracle_loss ||
                          iter == config.GetInt("iterations") - 1);
        if(config.GetBool("write_every_iter") || last_iter) {
            WriteModel(config.GetString("model_out"));
            if(last_iter)
                break;
        }
    }
}

void ReordererTrainer::InitializeModel(const ConfigTrainer & config) {
    srand(time(NULL));
    ofstream model_out(config.GetString("model_out").c_str());
    if(!model_out)
        THROW_ERROR("Must specify a valid model output with -model_out ('"
                        <<config.GetString("model_out")<<"')");
    attach_ = config.GetString("attach_null") == "left" ? 
                CombinedAlign::ATTACH_NULL_LEFT :
                CombinedAlign::ATTACH_NULL_RIGHT;
    combine_ = config.GetBool("combine_blocks") ? 
                CombinedAlign::COMBINE_BLOCKS :
                CombinedAlign::LEAVE_BLOCKS_AS_IS;
    features_.ParseConfiguration(config.GetString("feature_profile"));
    features_.SetMaxTerm(config.GetInt("max_term"));
    features_.SetUseReverse(config.GetBool("use_reverse"));
    non_local_features_.ParseConfiguration(config.GetString("non_local_feature_profile"));
    model_.SetCost(config.GetDouble("cost"));
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
