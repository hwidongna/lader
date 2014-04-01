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
using namespace std;
using namespace boost;

namespace lader {

// A task
class IParserGoldTask : public ShiftReduceTask {
public:
	IParserGoldTask(int id, const Sentence * data,
			const CombinedAlign * cal, const IParserRanks * ranks,
			vector<ReordererRunner::OutputType> * outputs,
			const ConfigBase& config, OutputCollector * collector) :
				ShiftReduceTask(id, "", NULL, NULL, outputs, config, collector),
				data_(data), cal_(cal), ranks_(ranks)	{ }
    void Run()
    {
        int sent = id_;
        int verbose = config_.GetInt("verbose");
        if(verbose >= 1){
            ess << endl << "Sentence " << sent << endl;
			ess << "Rank:";
			BOOST_FOREACH(int rank, ranks_->GetRanks())
				ess << " " << rank;
			ess << endl;
			ess << "Combined Align:";
			BOOST_FOREACH(CombinedAlign::Span span, cal_->GetSpans())
				ess << " [" << span.first << "," << span.second << "]";
			ess << endl;
		}
		ActionVector refseq = ranks_->GetReference(cal_);
		if (verbose >= 1){
			ess << "Reference:";
			BOOST_FOREACH(DPState::Action action, refseq)
				ess << " " << (char) action;
			ess << endl;
		}
		const Sentence & datas = (*data_);
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
	const Sentence * data_;
	const CombinedAlign * cal_;
	const IParserRanks * ranks_;
};

class IParserGold : public GoldStandard{
public:
	IParserGold() { }
	virtual ~IParserGold() {
		BOOST_FOREACH(CombinedAlign * cal, cals_)
			delete cal;
	}

	virtual void ReadAlignments(const std::string & align_in,
			std::vector<Ranks*> & ranks, std::vector<Sentence*> & datas) {
	    std::ifstream in(align_in.c_str());
	    if(!in) THROW_ERROR("Could not open alignment file: "
	                            <<align_in);
	    std::string line;
	    for (int i = 0 ; getline(in, line) ; i++){
	    	cals_.push_back(new CombinedAlign((*SafeAccess(datas,i))[0]->GetSequence(),
								Alignment::FromString(line),
								CombinedAlign::LEAVE_NULL_AS_IS, combine_, bracket_));
	        ranks.push_back(new
	            IParserRanks(CombinedAlign((*SafeAccess(datas,i))[0]->GetSequence(),
	                                Alignment::FromString(line),
	                                attach_, combine_, bracket_)));
	    }
	}
	virtual void Run(const ConfigBase & config){
		InitializeModel(config);
		ReadData(config.GetString("source_in"), data_);
		ReadAlignments(config.GetString("align_in"), ranks_, data_);
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
			else
				THROW_ERROR("Bad output format '" << str <<"'");
		}
		OutputCollector collector;
    	ThreadPool pool(config.GetInt("threads"), 1000);
    	for (int sent = 0 ; sent < data_.size() ; sent++) {
    		IParserRanks * ranks = dynamic_cast<IParserRanks*>(ranks_[sent]);
    		Task *task = new IParserGoldTask(sent, data_[sent], cals_[sent],
					ranks, &outputs, config, &collector);
    		pool.Submit(task);
    	}
    	pool.Stop(true);
	}
private:
	vector<CombinedAlign*> cals_;		// The combined alignment w/o attach null
};

}
#endif /* IPARSER_GOLD_H_ */
