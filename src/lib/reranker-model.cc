/*
 * reranker-model.cc
 *
 *  Created on: Feb 24, 2014
 *      Author: leona
 */

#include <reranker/reranker-model.h>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace reranker;
using namespace boost;

// IO Functions
void RerankerModel::ToStream(std::ostream & out, int threshold) {
    out << "reranker_model" << std::endl;
    for(int i = 0; i < (int)counts_.size(); i++)
        if(counts_[i] >= threshold)
            out << i << "\t" << feature_ids_.GetSymbol(i) << "\t" << counts_[i] << endl;
    out << endl;
}
RerankerModel * RerankerModel::FromStream(std::istream & in, bool renumber) {
    std::string line;
    RerankerModel * ret = new RerankerModel;
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
