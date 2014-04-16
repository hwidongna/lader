/*
 * shift-reduce-intermediate.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef IPARSER_TRAINER_H_
#define IPARSER_TRAINER_H_

#include <shift-reduce-dp/shift-reduce-trainer.h>
#include <shift-reduce-dp/iparser-model.h>
#include <shift-reduce-dp/iparser.h>
#include <shift-reduce-dp/iparser-ranks.h>
using namespace std;

namespace lader {


class IParserTrainer : public ShiftReduceTrainer {
	// A task
	class IParserTask : public Task {
	public:
		IParserTask(int id, const Sentence & data,
				IParserModel * model, FeatureSet * features, const ConfigBase& config,
				Parser::Result & result, OutputCollector & collector, vector<Parser::Result> & kbest) :
					id_(id), data_(data),
					model_(model), features_(features), config_(config),
					collector_(collector), result_(result), kbest_(kbest) { }
		void Run(){
			int verbose = config_.GetInt("verbose");
			int sent = id_;
			ostringstream err;
        	if(sent && sent % 100 == 0) err << ".";
        	if(sent && sent % (100*10) == 0) err << sent << endl;
			IParser parser(model_->GetMaxIns(), model_->GetMaxDel());
			parser.SetBeamSize(config_.GetInt("beam"));
			parser.SetVerbose(config_.GetInt("verbose"));
			parser.Search(*model_, *features_, data_);
			parser.GetKbestResult(kbest_);
			Parser::SetResult(result_, parser.GetBest());
			collector_.Write(id_, "", err.str());
		}
	protected:
		int id_;
		const Sentence & data_;
		IParserModel * model_; // The model
		FeatureSet * features_;  // The mapping on feature ids and which to use
		const ConfigBase& config_;
		Parser::Result & result_;
		OutputCollector & collector_;
		vector<Parser::Result> & kbest_;
	};
public:
	IParserTrainer() { }
	virtual ~IParserTrainer() { }
	// Initialize the model
	virtual void InitializeModel(const ConfigBase & config);
	void GetReferenceSequences(const std::string & align_in,
		std::vector<ActionVector> & refseq, std::vector<Sentence*> & datas){
	    std::ifstream in(align_in.c_str());
	    if(!in) THROW_ERROR("Could not open alignment file: "
	                            <<align_in);
	    std::string line;
	    int i = 0;
	    while(getline(in, line)){
	    	const vector<string> & srcs = (*SafeAccess(datas,i++))[0]->GetSequence();
			CombinedAlign cal(srcs, Alignment::FromString(line),
								CombinedAlign::LEAVE_NULL_AS_IS, combine_, bracket_);
			IParserRanks ranks(CombinedAlign(srcs, Alignment::FromString(line),
						attach_, combine_, bracket_), attach_trg_);
			ranks.Insert(&cal);
			refseq.push_back(ranks.GetReference(&cal));
	    }

	}
    // Train the reorderer incrementally, building they hypergraph each time
    // we parse
    virtual void TrainIncremental(const ConfigBase & config);
protected:
    vector<ActionVector> refseq_, dev_refseq_;
    CombinedAlign::NullHandler attach_trg_;

};

} /* namespace lader */


#endif /* IPARSER_TRAINER_H_ */
