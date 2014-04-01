/*
 * shift-reduce-model.h
 *
 *  Created on: Mar 6, 2014
 *      Author: leona
 */

#ifndef SHIFT_REDUCE_MODEL_H_
#define SHIFT_REDUCE_MODEL_H_

#include <lader/reorderer-model.h>
#include <boost/algorithm/string.hpp>

using namespace boost;

namespace lader{
class ShiftReduceModel : public ReordererModel {
public:
	ShiftReduceModel() : ReordererModel(), max_state_(1), max_swap_(0) {}
	virtual ~ShiftReduceModel() {}
	int GetMaxState() { return max_state_; }
	int GetMaxSwap() { return max_swap_; }
	void SetMaxState(int max_state) { max_state_ = max_state; }
	void SetMaxSwap(int max_swap) { max_swap_ = max_swap; }

	// IO Functions
	virtual void ToStream(std::ostream & out) {
	    out << "max_term " << max_term_ << endl;
	    out << "max_state " << max_state_ << endl;
	    out << "max_swap " << max_swap_ << endl;
	    out << "use_reverse " << use_reverse_ << endl;
	    out << "reorderer_model" << std::endl;
	    const vector<double> & weights = GetWeights();
	    for(int i = 0; i < (int)weights.size(); i++)
	        if(abs(weights[i]) > MINIMUM_WEIGHT)
	            out << feature_ids_.GetSymbol(i) << "\t" << weights[i] << endl;
	    out << endl;
	}
	static ReordererModel * FromStream(std::istream & in) {
	    std::string line;
	    ShiftReduceModel * ret = new ShiftReduceModel;
	    GetConfigLine(in, "max_term", line);
		ret->SetMaxTerm(atoi(line.c_str()));
		GetConfigLine(in, "max_state", line);
		ret->SetMaxState(atoi(line.c_str()));
		GetConfigLine(in, "max_swap", line);
		ret->SetMaxSwap(atoi(line.c_str()));
		GetConfigLine(in, "use_reverse", line);
		ret->SetUseReverse(line == "true" || line == "1");
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
	int max_state_;	// The maximum number of states in equality checking
	int max_swap_;	// The maximum number of swap actions for non-ITG
};
}

#endif /* SHIFT_REDUCE_MODEL_H_ */
