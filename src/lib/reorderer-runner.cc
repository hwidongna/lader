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
    int verbose = config_.GetInt("verbose");
    // Load the data
	if (verbose > 1)
		cerr << "Sentence " << id_ << endl;
    std::vector<FeatureDataBase*> datas = features_->ParseInput(line_);
    // Save the original string
    vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
    // Build the hypergraph
    graph_->BuildHyperGraph(*model_, *features_, *non_local_features_, datas);
    // Reorder
    std::vector<int> reordering;
    graph_->GetBest()->GetReordering(reordering);
    datas[0]->Reorder(reordering);
    // Print the string
    ostringstream out;
    for(int i = 0; i < (int)outputs_->size(); i++) {
        if(i != 0) out << "\t";
        if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
            out << datas[0]->ToString();
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_PARSE) {
            graph_->GetBest()->PrintParse(words, out);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_HYPERGRAPH) {
            graph_->PrintHyperGraph(words, out);
        } else if(outputs_->at(i) == ReordererRunner::OUTPUT_ORDER) {
            for(int j = 0; j < (int)reordering.size(); j++) {
                if(j != 0) out << " ";
                out << reordering[j];
            }
        } else {
            THROW_ERROR("Unimplemented output format");
        }
    }
    out << endl;
    ostringstream err;
    err << "Decoding " << id_+1 << "th sentence" << endl;
    collector_->Write(id_, out.str(), err.str());
    // Clean up the data
    BOOST_FOREACH(FeatureDataBase* data, datas)
        delete data;
    delete graph_;
}

// Run the model
void ReordererRunner::Run(const ConfigRunner & config) {
    InitializeModel(config);
    // Create the thread pool
    ThreadPool pool(config.GetInt("threads"), 1000);
    OutputCollector collector;

    std::string line;
    std::string source_in = config.GetString("source_in");
    std::ifstream in(source_in.c_str());
    int id = 0;
    int gapSize = config.GetInt("gap-size");
    int beam = config.GetInt("beam");
    int pop_limit = config.GetInt("pop_limit");
    int max_seq = config.GetInt("max-seq");
    bool cube_growing = config.GetBool("cube_growing");
    bool mp = config.GetBool("mp");
    bool full_fledged = config.GetBool("full_fledged");
    int verbose = config.GetInt("verbose");
    DiscontinuousHyperGraph graph(gapSize, max_seq, cube_growing, full_fledged, mp, verbose) ;
    graph.SetBeamSize(beam);
	graph.SetPopLimit(pop_limit);
    if (config.GetString("bigram").length())
		graph.LoadLM(config.GetString("bigram").c_str());
    while(std::getline(in != NULL? in : std::cin, line)) {
    	ReordererTask *task = new ReordererTask(id++, line, model_, features_,
				non_local_features_, &outputs_, config, graph.Clone(),
				&collector);
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
