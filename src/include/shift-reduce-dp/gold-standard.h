/*
 * gold-standard.h
 *
 *  Created on: Mar 7, 2014
 *      Author: leona
 */

#ifndef GOLD_STANDARD_H_
#define GOLD_STANDARD_H_

#include <lader/reorderer-trainer.h>
#include <lader/reorderer-runner.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/ddpstate.h>
#include <lader/feature-data-sequence.h>
#include <shift-reduce-dp/flat-tree.h>
#include <shift-reduce-dp/config-gold.h>
#include <boost/tokenizer.hpp>
#include <shift-reduce-dp/shift-reduce-runner.h>
using namespace std;
using namespace boost;

namespace lader {
// A task
class ShiftReduceGoldTask : public ShiftReduceTask, public ReordererEvaluator {
public:
	ShiftReduceGoldTask(int id, const string & sline, const string & aline,
			FeatureSet * features, vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				ShiftReduceTask(id, "", NULL, features, outputs, config, collector),
				sline_(sline), aline_(aline)	{ }
	void Run()
	{
		ReordererEvaluator::InitializeModel(config_);
		Sentence datas = features_->ParseInput(sline_);
		const vector<string> & srcs = datas[0]->GetSequence();
		int sent = id_;
		int verbose = config_.GetInt("verbose");
		if(verbose >= 1)
			ess << endl << "Sentence " << sent << endl;
		Alignment al = Alignment::FromString(aline_);
		CombinedAlign cal(srcs, al, attach_, combine_, bracket_);
		Ranks ranks(cal);
		ActionVector refseq = ranks.GetReference();
		if (verbose >= 1){
			ess << "Reference:";
			BOOST_FOREACH(DPState::Action action, refseq)
				ess << " " << (char) action;
			ess << endl;
		}
		int n = datas[0]->GetNumWords();
		if (refseq.size() < 2*n - 1)
			THROW_ERROR("Fail to get correct reference sequence" << endl)
		DParser p(n);
		DPState * state = p.GuidedSearch(refseq, n);
		Output(datas, state, &al, &cal);
		collector_->Write(id_, oss.str(), ess.str());
	}
protected:
	const string sline_;
	const string aline_;
};

class GoldStandard : public ReordererEvaluator{
public:
	GoldStandard(): features_(NULL) { }
	virtual ~GoldStandard() {
	    if(features_) delete features_;
	}

	// Initialize the model
	virtual void InitializeModel(const ConfigBase & config){
		ReordererEvaluator::InitializeModel(config);
		features_ = new FeatureSet;
		features_->ParseConfiguration(config.GetString("feature_profile"));
	}

	virtual Task * NewGoldTask(int sent, const string & sline,
			const string & aline, vector<ReordererRunner::OutputType> & outputs,
			const ConfigBase& config, OutputCollector & collector) {
		return new ShiftReduceGoldTask(sent, sline, aline,
				features_, &outputs, config, &collector);
	}

	virtual void Run(const ConfigBase & config){
		InitializeModel(config);
		vector<ReordererRunner::OutputType> outputs;
		tokenizer<char_separator<char> > outs(config.GetString("out_format"),
				char_separator<char>(","));
		BOOST_FOREACH(const string & str, outs) {
			if(str == "string")
				outputs.push_back(ReordererRunner::OUTPUT_STRING);
			else if(str == "parse")
				outputs.push_back(ReordererRunner::OUTPUT_PARSE);
			else if(str == "order")
				outputs.push_back(ReordererRunner::OUTPUT_ORDER);
			else if(str == "flatten")
				outputs.push_back(ReordererRunner::OUTPUT_FLATTEN);
			else if(str == "action")
				outputs.push_back(ReordererRunner::OUTPUT_ACTION);
	        else if (str == "align")
				outputs.push_back(ReordererRunner::OUTPUT_ALIGN);
			else
				THROW_ERROR("Bad output format '" << str <<"'");
		}
		OutputCollector collector;
		string source_in = config.GetString("source_in");
		string align_in = config.GetString("align_in");
		std::ifstream sin(source_in.c_str());
		if(!sin) THROW_ERROR("Could not open source file: "
				<<source_in);
	    std::ifstream ain(align_in.c_str());
	    if(!ain) THROW_ERROR("Could not open alignment file: "
	                            <<align_in);
    	ThreadPool pool(config.GetInt("threads"), 1000);
    	string sline, aline;
    	for (int sent = 0 ; getline(sin, sline) && getline(ain, aline) ; sent++) {
    		Task *task = NewGoldTask(sent, sline, aline, outputs, config, collector);
    		pool.Submit(task);
    	}
    	pool.Stop(true);
	}
protected:
    FeatureSet* features_;  // The mapping on feature ids and which to use
};

}
#endif /* GOLD_STANDARD_H_ */
