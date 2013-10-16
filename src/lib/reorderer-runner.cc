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
	Sentence source = features_->ParseInput(sline_);
    // Save the original string
    vector<string> words = ((FeatureDataSequence*)source[0])->GetSequence();
    Sentence target;
    CombinedAlign align;
	if (bilingual_features_){
		target = bilingual_features_->ParseInput(tline_);
		Alignment alignment = Alignment::FromString(aline_);
		CombinedAlign::NullHandler attach_ = config_.GetString("attach_null") == "left" ?
	                CombinedAlign::ATTACH_NULL_LEFT :
	                CombinedAlign::ATTACH_NULL_RIGHT;
		CombinedAlign::BlockHandler combine_ = config_.GetBool("combine_blocks") ?
	                CombinedAlign::COMBINE_BLOCKS :
	                CombinedAlign::LEAVE_BLOCKS_AS_IS;
		CombinedAlign::BracketHandler bracket_ = config_.GetBool("combine_brackets") ?
					CombinedAlign::ALIGN_BRACKET_SPANS :
					CombinedAlign::LEAVE_BRACKETS_AS_IS;
		align = CombinedAlign(words, alignment, attach_, combine_, bracket_);
		graph_->SetBilingual(bilingual_features_, &target, &align);
	}
    // Build the hypergraph
    graph_->BuildHyperGraph(*model_, *features_, source);
    // Reorder
    std::vector<int> reordering;
    graph_->GetBest()->GetReordering(reordering);
    source[0]->Reorder(reordering);
    // Print the string
    ostringstream out;
    for(int i = 0; i < (int)outputs_->size(); i++) {
        if(i != 0) out << "\t";
        if(outputs_->at(i) == ReordererRunner::OUTPUT_STRING) {
            out << source[0]->ToString();
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
    BOOST_FOREACH(FeatureDataBase* data, source)
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
    std::ifstream sin(source_in.c_str());
    if (!sin)
    	cerr << "use stdin for source_in" << endl;
    std::ifstream tin, ain;
    if (!config.GetString("target_in").empty()){
    	tin.open(config.GetString("target_in").c_str(), ios_base::in);
        ain.open(config.GetString("align_in").c_str(), ios_base::in);
    }
    int id = 0;
    int gapSize = config.GetInt("gap-size");
    int beam = config.GetInt("beam");
    int pop_limit = config.GetInt("pop_limit");
    bool cube_growing = config.GetBool("cube_growing");
    bool mp = config.GetBool("mp");
    bool full_fledged = config.GetBool("full_fledged");
    int verbose = config.GetInt("verbose");
    DiscontinuousHyperGraph graph(gapSize, cube_growing, full_fledged, mp, verbose) ;
    graph.SetBeamSize(beam);
	graph.SetPopLimit(pop_limit);
    while(std::getline(sin != NULL? sin : std::cin, line)) {
    	ReordererTask *task = new ReordererTask(id++, line, model_, features_,
    		 	&outputs_, config, graph.Clone(),
				&collector);
		if (bilingual_features_){
			std::string tline, aline;
			std::getline(tin, tline);
			std::getline(ain, aline);
			task->SetBilingual(bilingual_features_, tline, aline);
		}
        pool.Submit(task);
    }
    if (sin) sin.close();
    pool.Stop(true); 
}

// Initialize the model
void ReordererRunner::InitializeModel(const ConfigRunner & config) {
	std::ifstream in(config.GetString("model").c_str());
	if(!in) THROW_ERROR("Couldn't read model from file (-model '"
						<< config.GetString("model") << "')");
	features_ = FeatureSet::FromStream(in);
	if (!config.GetBool("backward_compatibility"))
		bilingual_features_ = FeatureSet::FromStream(in);
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
        else
            THROW_ERROR("Bad output format '" << str <<"'");
    }
}
