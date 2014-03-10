/*
 * reranker-model.cc
 *
 *  Created on: Feb 24, 2014
 *      Author: leona
 */

#include <reranker/reranker-model.h>
#include <boost/algorithm/string.hpp>
#include <cstdio>

using namespace std;
using namespace reranker;
using namespace boost;

// IO Functions
void RerankerModel::ToStream(std::ostream & out, int threshold) {
	out << "max_swap " << max_swap_ << endl;
    out << "reranker_model" << std::endl;
    for(int i = 0; i < (int)counts_.size(); i++)
        if(counts_[i] >= threshold)
            out << i << "\t" << feature_ids_.GetSymbol(i) << "\t" << counts_[i] << endl;
    out << endl;
}
RerankerModel * RerankerModel::FromStream(std::istream & in, bool renumber) {
    std::string line;
    RerankerModel * ret = new RerankerModel;
    GetConfigLine(in, "max_swap", line);
    ret->SetMaxSwap(atoi(line.c_str()));
    GetlineEquals(in, "reranker_model");
    while(std::getline(in, line) && line.length()) {
        vector<string> columns;
        algorithm::split(columns, line, is_any_of("\t"));
        if(columns.size() != 3)
            THROW_ERROR("Bad line in reordered model: " << line);
        int id, count;
        std::istringstream iss(columns[0]);
        iss >> id;
        std::istringstream iss2(columns[2]);
        iss2 >> count;
        ret->SetCount(id, columns[1], count, renumber);
    }
    return ret;
}

void RerankerModel::SetWeightsFromStream(std::istream & in) {
	std::string line;
	int idx;
	double w;
	while(std::getline(in, line) && line.length()) {
		sscanf(line.c_str(), "%d=%lf", &idx, &w);
		if((int)v_.size() <= idx)
			v_.resize(idx+1,0.0);
		v_[idx] = w;
	}
}
