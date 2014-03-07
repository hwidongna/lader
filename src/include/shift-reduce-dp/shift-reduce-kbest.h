/*
 * shift-reduce-runner.h
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */

#ifndef SHIFT_REDUCE_KBEST_H_
#define SHIFT_REDUCE_KBEST_H_

#include <shift-reduce-dp/shift-reduce-runner.h>
#include <lader/feature-set.h>
#include <shift-reduce-dp/dparser.h>
#include <lader/feature-data-sequence.h>
#include <reranker/flat-tree.h>

using namespace std;
namespace lader {

// A task
class ShiftReduceKbestTask : public ShiftReduceTask {
public:
	ShiftReduceKbestTask(int id, const string & line,
				  ShiftReduceModel * model, FeatureSet * features,
				  vector<ReordererRunner::OutputType> * outputs,
				  const ConfigBase& config, OutputCollector * collector) :
					  ShiftReduceTask(id, line, model, features, outputs, config, collector) {}
	void Run(){
		int verbose = config_.GetInt("verbose");
		int sent = id_;
		// Load the data
		Sentence datas = features_->ParseInput(line_);
		// Save the original string
		vector<string> words = ((FeatureDataSequence*)datas[0])->GetSequence();
		// Build a reordering tree
		if (verbose >= 1)
			ess << endl << "Sentence " << sent << endl;
		Parser * p;
		if (model_->GetMaxSwap() > 0)
			p = new DParser(model_->GetMaxSwap());
		else
			p = new Parser();
		p->SetBeamSize(config_.GetInt("beam"));
		p->SetVerbose(config_.GetInt("verbose"));
		p->Search(*model_, *features_, datas);
		vector<Parser::Result> kbest;
		p->GetKbestResult(kbest);
		oss << kbest.size() << endl;
		for (int k = 0 ; k < kbest.size() ; k++){
			Output(kbest[k], datas, p->GetKthBest(k));
		}
		collector_->Write(id_, oss.str(), ess.str());
		// Clean up the data
		BOOST_FOREACH(FeatureDataBase* data, datas)
			delete data;
		delete p;
	}
};

class ShiftReduceKbest : public ShiftReduceRunner{
public:
	virtual Task * NewShiftReduceTask(int id, const string & line,
			const ConfigBase& config) {
		return new ShiftReduceKbestTask(id, line,
				dynamic_cast<ShiftReduceModel*>(model_), features_, &outputs_,
				config, &collector_);
	}
};

} /* namespace lader */
#endif /* SHIFT_REDUCE_KBEST_H_ */
