#include <lader/reorderer-runner.h>
#include <lader/feature-data-sequence.h>
#include <lader/thread-pool.h>
#include <lader/output-collector.h>
#include <boost/tokenizer.hpp>
#include <lader/hyper-graph.h>
#include "shift-reduce-dp/flat-tree.h"

using namespace lader;
using namespace std;
using namespace boost;

void ReordererTask::Run() {
    int verbose = config_.GetInt("verbose");
    // Load the data
    Sentence datas = features_->ParseInput(line_);
    // Save the original string
    vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
    struct timespec search={0,0};
    struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    // Build the hypergraph
    graph_->SetAllStacks(datas[0]->GetNumWords());
    graph_->BuildHyperGraph(*model_, *features_, datas);
    clock_gettime(CLOCK_MONOTONIC, &tend);
    search.tv_sec += tend.tv_sec - tstart.tv_sec;
    search.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
    ostringstream ess;
    ess << std::setprecision(5) << id_ << " " << words.size()
    	<< " " << ((double)search.tv_sec + 1.0e-9*search.tv_nsec)
    	<< " " << graph_->NumParses() << endl;
    // Reorder
    std::vector<int> reordering;
    graph_->GetBest()->GetReordering(reordering);
    datas[0]->Reorder(reordering);
    // Print the string
    ostringstream oss;
    for(int i = 0; i < (int)outputs_->size(); i++) {
        if(i != 0) oss << "\t";
        if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
            oss << datas[0]->ToString();
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_PARSE) {
            graph_->GetBest()->PrintParse(words, oss);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_HYPERGRAPH) {
            graph_->PrintHyperGraph(words, oss);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_ORDER) {
            for(int j = 0; j < (int)reordering.size(); j++) {
                if(j != 0) oss << " ";
                oss << reordering[j];
            }
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_SCORE) {
        	oss << graph_->GetBest()->GetScore();
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_FLATTEN) {
        	HypNode dummy(0, words.size(), NULL, HyperEdge::EDGE_ROOT);
        	HypNode * root = dummy.Flatten(graph_->GetBest()->GetLeftHyp());
        	root->PrintParse(words, oss);
        } else {
            THROW_ERROR("Unimplemented output format");
        }
    }
    oss << endl;
    collector_->Write(id_, oss.str(), ess.str());
    // Clean up the data
    BOOST_FOREACH(FeatureDataBase* data, datas)
        delete data;
    delete graph_;
}

// Run the model
void ReordererRunner::Run(const ConfigBase & config) {
    InitializeModel(config);
    // Create the thread pool
    ThreadPool pool(config.GetInt("threads"), 1000);

    std::string line;
    std::string source_in = config.GetString("source_in");
    std::ifstream sin(source_in.c_str());
    if (!sin)
    	cerr << "use stdin for source_in" << endl;
    int id = 0;
    HyperGraph graph(config.GetBool("cube_growing")) ;
    graph.SetBeamSize(config.GetInt("beam"));
	graph.SetPopLimit(config.GetInt("pop_limit"));
	// do not need to set threads because it runs in parallel at sentence-level
	cerr << "Sentence Length Time #Parse" << endl;
    while(std::getline(sin != NULL? sin : std::cin, line)) {
    	ReordererTask *task = new ReordererTask(id++, line, model_, features_,
    		 	&outputs_, config, graph.Clone(),
				&collector_);
		pool.Submit(task);
    }
    if (sin) sin.close();
    pool.Stop(true); 
}

// Initialize the model
void ReordererRunner::InitializeModel(const ConfigBase & config) {
	std::ifstream in(config.GetString("model").c_str());
	if(!in) THROW_ERROR("Couldn't read model from file (-model '"
						<< config.GetString("model") << "')");
	features_ = FeatureSet::FromStream(in);
	model_ = ReordererModel::FromStream(in);
    model_->SetAdd(false);
    tokenizer<char_separator<char> > outs(config.GetString("out_format"),
                                          char_separator<char>(","));
    BOOST_FOREACH(const string & str, outs) {
        if(str == "string")
            outputs_.push_back(OUTPUT_STRING);
        else if(str == "parse")
            outputs_.push_back(OUTPUT_PARSE);
        else if(str == "hypergraph")
            outputs_.push_back(OUTPUT_HYPERGRAPH);
        else if(str == "order")
            outputs_.push_back(OUTPUT_ORDER);
        else if(str == "score")
        	outputs_.push_back(OUTPUT_SCORE);
        else if(str == "flatten")
        	outputs_.push_back(OUTPUT_FLATTEN);
        else if(str == "action")
        	outputs_.push_back(OUTPUT_ACTION);
        else
            THROW_ERROR("Bad output format '" << str <<"'");
    }
}
