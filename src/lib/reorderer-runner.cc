#include <lader/reorderer-runner.h>
#include <lader/feature-data-sequence.h>
#include <lader/thread-pool.h>
#include <lader/output-collector.h>
#include <boost/tokenizer.hpp>
#include <lader/discontinuous-hyper-graph.h>

using namespace lader;
using namespace std;
using namespace boost;

void ReordererTask::Run() {
    // Load the data
	if (verbose_ > 1)
		cerr << "Sentence " << id_ << endl;
    std::vector<FeatureDataBase*> datas = features_->ParseInput(line_);
    // Save the original string
    vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
    // Build the hypergraph
    HyperGraph * hyper_graph = new DiscontinuousHyperGraph(gap_, mp_, full_fledged_, verbose_);
    hyper_graph->BuildHyperGraph(*model_, *features_, *non_local_features_, datas, beam_);
    // Reorder
    std::vector<int> reordering;
    hyper_graph->GetReordering(reordering, hyper_graph->GetBest());
    datas[0]->Reorder(reordering);
    // Print the string
    ostringstream oss;
    for(int i = 0; i < (int)outputs_->size(); i++) {
        if(i != 0) oss << "\t";
        if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
            oss << datas[0]->ToString();
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_PARSE) {
            hyper_graph->GetBest()->PrintParse(words, oss);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_HYPERGRAPH) {
            hyper_graph->PrintHyperGraph(words, oss);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_ORDER) {
            for(int j = 0; j < (int)reordering.size(); j++) {
                if(j != 0) oss << " ";
                oss << reordering[j];
            }
        } else {
            THROW_ERROR("Unimplemented output format");
        }
    }
    delete hyper_graph;
    oss << endl;
    collector_->Write(id_, oss.str(), "");
    // Clean up the data
    BOOST_FOREACH(FeatureDataBase* data, datas)
        delete data;
}

// Run the model
void ReordererRunner::Run(const ConfigRunner & config) {
    InitializeModel(config);
    // Create the thread pool
    ThreadPool pool(config.GetInt("threads"), 1000);
    OutputCollector collector;

    std::string line;
    int id = 0, beam = config.GetInt("beam");
    int gapSize = config.GetInt("gap-size");
    bool full_fledged = config.GetBool("full_fledged");
    bool mp = config.GetBool("mp");
    int verbose = config.GetInt("verbose");
    std::string source_in = config.GetString("source_in");
    std::ifstream in(source_in.c_str());
    if (gapSize > 0)
    	cerr << "use discontinuous hyper-graph: D=" << gapSize << endl;
    if (mp)
        cerr << "enable monotone at punctuation to prevent excessive reordering" << endl;
    while(std::getline(in != NULL? in : std::cin, line)) {
    	ReordererTask *task = new ReordererTask(id++, line, model_, features_, non_local_features_, &outputs_,
    			beam, &collector, gapSize, mp, full_fledged, verbose);
        pool.Submit(task);
    }
    if (in) in.close();
    pool.Stop(true); 
}

// Initialize the model
void ReordererRunner::InitializeModel(const ConfigRunner & config) {
    ReadModel(config.GetString("model"), config.GetBool("backward_compatibility"));
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
        else
            THROW_ERROR("Bad output format '" << str <<"'");
    }
}
