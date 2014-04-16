/*
 * iparser-runner.h
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#ifndef IPARSER_RUNNER_H_
#define IPARSER_RUNNER_H_

#include <shift-reduce-dp/shift-reduce-runner.h>
#include <shift-reduce-dp/iparser-model.h>
#include <shift-reduce-dp/iparser.h>
using namespace std;
namespace lader {


// A task
class IParserTask : public ShiftReduceTask {
public:
	IParserTask(int id, const string & line,
			ShiftReduceModel * model, FeatureSet * features,
			vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				ShiftReduceTask(id, line, model, features, outputs, config, collector) { }
	virtual DPStateNode * NewDPStateNode(vector<string> & words){
		return new IDPStateNode(0, words.size(), NULL, DPState::INIT);
	}
    void Run()
    {
        int sent = id_;
        int verbose = config_.GetInt("verbose");
        // Load the data
        Sentence datas = features_->ParseInput(line_);
        // Save the original string
        vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
        // Build a reordering tree
        if(verbose >= 1)
            ess << endl << "Sentence " << sent << endl;

        IParserModel * model = dynamic_cast<IParserModel*>(model_);
        struct timespec search={0,0};
        struct timespec tstart={0,0}, tend={0,0};
        clock_gettime(CLOCK_MONOTONIC, &tstart);
        IParser parser(model->GetMaxIns(), model->GetMaxDel());
        parser.SetBeamSize(config_.GetInt("beam"));
        parser.SetVerbose(config_.GetInt("verbose"));
        parser.Search(*model, *features_, datas);
        clock_gettime(CLOCK_MONOTONIC, &tend);
        search.tv_sec += tend.tv_sec - tstart.tv_sec;
        search.tv_nsec += tend.tv_nsec - tstart.tv_nsec;
        ess << std::setprecision(5) << id_ << " " << words.size()
			<< " " << ((double)search.tv_sec + 1.0e-9*search.tv_nsec)
        	<< " " << parser.GetNumEdges() << " " << parser.GetNumStates() << " " << parser.GetNumParses() << endl;

        DPState *best = parser.GetBest();
        if (!best){
        	if (verbose >= 1)
        		ess << "Fail to produce the parsing result" << endl;
        	oss << endl;
        	collector_->Write(id_, oss.str(), ess.str());
        	return;
        }
        if(verbose >= 1){
        	Parser::Result result;
        	Parser::SetResult(result, best);
			ess << "Result Purmutation:";
			BOOST_FOREACH(int order, result.order)
				ess << " " << order;
			ess << endl;
        }
        Output(datas, best);
		collector_->Write(id_, oss.str(), ess.str());
		// Clean up the data
		BOOST_FOREACH(FeatureDataBase* data, datas){
			delete data;
		}
	}
};

class IParserRunner : public ShiftReduceRunner{
public:
	// Initialize the model
	virtual void InitializeModel(const ConfigBase & config) {
		std::ifstream in(config.GetString("model").c_str());
		if(!in) THROW_ERROR("Couldn't read model from file (-model '"
							<< config.GetString("model") << "')");
		features_ = FeatureSet::FromStream(in);
		model_ = IParserModel::FromStream(in);
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
	        else if (str == "action")
	        	outputs_.push_back(OUTPUT_ACTION);
	        else
	            THROW_ERROR("Bad output format '" << str <<"'");
	    }
	}
	virtual Task * NewShiftReduceTask(int id, const string & line,
			const ConfigBase& config) {
		return new IParserTask(id, line,
				dynamic_cast<IParserModel*>(model_), features_, &outputs_,
				config, &collector_);
	}
};

} /* namespace lader */


#endif /* IPARSER_RUNNER_H_ */
