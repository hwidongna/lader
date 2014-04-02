/*
 * iparser-model.h
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#ifndef IPARSER_MODEL_H_
#define IPARSER_MODEL_H_

#include <shift-reduce-dp/shift-reduce-model.h>
#include <boost/algorithm/string.hpp>

using namespace boost;

namespace lader{
class IParserModel : public ShiftReduceModel {
public:
	IParserModel() : ShiftReduceModel(), max_ins_(0), max_del_(0) {}
	virtual ~IParserModel() {}
	int GetMaxIns() { return max_ins_; }
	int GetMaxDel() { return max_del_; }
	void SetMaxIns(int max_ins) { max_ins_ = max_ins; }
	void SetMaxDel(int max_del) { max_del_ = max_del; }

	// IO Functions
	virtual void ToStream(std::ostream & out) {
	    out << "max_state " << max_state_ << endl;
	    out << "max_ins " << max_ins_ << endl;
	    out << "max_del " << max_del_ << endl;
	    out << "reorderer_model" << std::endl;
	    const vector<double> & weights = GetWeights();
	    for(int i = 0; i < (int)weights.size(); i++)
	        if(abs(weights[i]) > MINIMUM_WEIGHT)
	            out << feature_ids_.GetSymbol(i) << "\t" << weights[i] << endl;
	    out << endl;
	}
	static ReordererModel * FromStream(std::istream & in) {
	    std::string line;
	    IParserModel * ret = new IParserModel;
		GetConfigLine(in, "max_state", line);
		ret->SetMaxState(atoi(line.c_str()));
		GetConfigLine(in, "max_ins", line);
		ret->SetMaxIns(atoi(line.c_str()));
		GetConfigLine(in, "max_del", line);
		ret->SetMaxDel(atoi(line.c_str()));
	    GetlineEquals(in, "reorderer_model");
	    while(std::getline(in, line) && line.length()) {
	        vector<string> columns;
	        algorithm::split(columns, line, is_any_of("\t"));
	        if(columns.size() != 2)
	            THROW_ERROR("Bad line in reordered model: " << line);
	        double dub;
	        std::istringstream iss(columns[1]);
	        iss >> dub;
	        ret->SetWeight(columns[0], dub);
	    }
	    return ret;
	}
private:
	int max_ins_, max_del_;	// The maximum number of insert/delete actions
};
}


#endif /* IPARSER_MODEL_H_ */
