/*
 * iparser-gold.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef IPARSER_GOLD_H_
#define IPARSER_GOLD_H_

#include <shift-reduce-dp/gold-standard.h>
#include <shift-reduce-dp/iparser.h>
#include <shift-reduce-dp/iparser-ranks.h>
#include <shift-reduce-dp/iparser-evaluator.h>
#include <shift-reduce-dp/idpstate.h>
using namespace std;
using namespace boost;

namespace lader {

// A task
class IParserGoldTask : public ShiftReduceTask, public IParserEvaluator {
public:
	IParserGoldTask(int id, const string & sline, const string & aline,
			FeatureSet * features, vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				ShiftReduceTask(id, "", NULL, features, outputs, config, collector),
				sline_(sline), aline_(aline) { }
	virtual DPStateNode * NewDPStateNode(vector<string> & words){
		return new IDPStateNode(0, words.size(), NULL, DPState::INIT);
	}
    void Run()
    {
    	IParserEvaluator::InitializeModel(config_);
    	Sentence datas = features_->ParseInput(sline_);
    	const vector<string> & srcs = datas[0]->GetSequence();
        int sent = id_;
        int verbose = config_.GetInt("verbose");
        if(verbose >= 1)
            ess << endl << "Sentence " << sent << endl;
    	CombinedAlign cal(srcs, Alignment::FromString(aline_),
							CombinedAlign::LEAVE_NULL_AS_IS, combine_, bracket_);
        IParserRanks ranks(CombinedAlign(srcs, Alignment::FromString(aline_),
                	attach_, combine_, bracket_), attach_trg_);
        ranks.Insert(&cal);
		ActionVector refseq = ranks.GetReference(&cal);
		if (verbose >= 1){
			ess << "Reference:";
			BOOST_FOREACH(DPState::Action action, refseq)
				ess << " " << (char) action;
			ess << endl;
		}
		int n = datas[0]->GetNumWords();
		if (refseq.size() < 2*n - 1){
			ess << "Fail to get correct reference sequence" << endl;
			oss << endl;
			collector_->Write(id_, oss.str(), ess.str());
			return;
		}

		IParser p(n, n);
		DPState * state = p.GuidedSearch(refseq, n);
		Output(datas, state);
		collector_->Write(id_, oss.str(), ess.str());
	}
protected:
	const string sline_;
	const string aline_;
};

class IParserGold : public GoldStandard{
public:
	IParserGold() { }
	virtual ~IParserGold() { }

	virtual Task * NewGoldTask(int sent, const string & sline,
			const string & aline, vector<ReordererRunner::OutputType> & outputs,
			const ConfigBase& config, OutputCollector & collector) {
		return new IParserGoldTask(sent, sline, aline,
				features_, &outputs, config, &collector);
	}
};

}
#endif /* IPARSER_GOLD_H_ */
