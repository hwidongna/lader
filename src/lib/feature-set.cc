
#include <lader/feature-set.h>
#include <boost/algorithm/string.hpp>

using namespace lader;
using namespace std;
using namespace boost;


// Generates the features that can be factored over a node
FeatureVectorInt * FeatureSet::MakeEdgeFeatures(
        const vector<FeatureDataBase*> & sent,
        const HyperEdge & edge,
        SymbolSet<int> & feature_ids,
        bool add) const {
    // Otherwise generate the features
    FeatureVectorInt * feats = new FeatureVectorInt;
    for(int i = 0; i < (int)sent.size(); i++)
        feature_gens_[i]->GenerateEdgeFeatures(*sent[i], edge, feature_ids, add, *feats);
    return feats;
}

// Generates the features that can be factored over a hypothesis
FeatureVectorInt * FeatureSet::MakeNonLocalFeatures(
        const vector<FeatureDataBase*> & sent,
        const Hypothesis & left,
	    const Hypothesis & right,
        SymbolSet<int> & feature_ids,
        bool add) const {
    // Otherwise generate the features
    FeatureVectorInt * feats = new FeatureVectorInt;
    // non-local feature is not defined for parse tree
    for(int i = 0; i < (int)sent.size() && i < (int)feature_gens_.size(); i++)
        feature_gens_[i]->GenerateNonLocalFeatures(*sent[i], left, right, feature_ids, add, *feats);
    return feats;
}

vector<FeatureDataBase*> FeatureSet::ParseInput(const string & line) const {
    vector<string> columns;
    algorithm::split(columns, line, is_any_of("\t"));
    if(feature_gens_.size() != columns.size()) {
        THROW_ERROR("Number of columns in feature file ("<<columns.size()<<
                    ") didn't equal feature profile ("<<feature_gens_.size()<<
                    ") at"<<endl<<line<<endl);
    }
    vector<FeatureDataBase*> ret(columns.size());
    for(int i = 0; i < (int)columns.size(); i++) {
        ret[i] = feature_gens_[i]->ParseData(columns[i]);
    }
    return ret;
}

void FeatureSet::ParseConfiguration(const string & str) {
    config_str_ = str;
    // Split configurations into sizes
    vector<string> configs;
    algorithm::split(configs, str, is_any_of("|"));
    feature_gens_ = vector<FeatureBase*>(configs.size());
    // Iterate over multiple pipe-separated config strings of format type=config
    for(int i = 0; i < (int)configs.size(); i++) {
        size_t pos = configs[i].find_first_of("=");
        if(pos == string::npos || pos == 0 || pos == configs[i].length()-1)
            THROW_ERROR("Bad configuration string " << str);
        string type = configs[i].substr(0, pos);
        feature_gens_[i] = FeatureBase::CreateNew(type);
        string sub_config = configs[i].substr(pos+1);
        feature_gens_[i]->ParseConfiguration(sub_config);
    }
}

// IO Functions
void FeatureSet::ToStream(ostream & out) {
    out << "feature_set" << endl;
    out << "config_str " << config_str_ << endl;
    out << "max_term " << max_term_ << endl;
    out << "use_reverse " << use_reverse_ << endl;
    out << endl;
}
FeatureSet * FeatureSet::FromStream(istream & in) {
    string line;
    GetlineEquals(in, "feature_set");
    string config;
    GetConfigLine(in, "config_str", config);
    FeatureSet * ret = new FeatureSet;
    ret->ParseConfiguration(config);
    GetConfigLine(in, "max_term", config);
    ret->SetMaxTerm(atoi(config.c_str()));
    GetConfigLine(in, "use_reverse", config);
    ret->SetUseReverse(config == "true" || config == "1");
    GetlineEquals(in, "");
    return ret;
}
