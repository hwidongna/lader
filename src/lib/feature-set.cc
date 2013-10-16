
#include <lader/feature-set.h>
#include <boost/algorithm/string.hpp>
#include <lader/feature-bilingual.h>

using namespace lader;
using namespace std;
using namespace boost;


// Generates the features that can be factored over a node
FeatureVectorInt * FeatureSet::MakeEdgeFeatures(
		const Sentence & sent,
        const HyperEdge & edge,
        SymbolSet<int> & feature_ids,
        bool add) const {
    FeatureVectorInt * feats = new FeatureVectorInt;
    for(int i = 0; i < (int)sent.size(); i++)
        feature_gens_[i]->GenerateEdgeFeatures(*sent[i], edge, feature_ids, add, *feats);
    return feats;
}

// Generates the biligual features that can be factored over a node
// This is useful to accumulate features to the existing vector (ret)
FeatureVectorInt *FeatureSet::MakeBilingualFeatures(
		const Sentence & source,
		const Sentence & target,
		const CombinedAlign & align,
		const HyperEdge & edge,
		SymbolSet<int> & feature_ids,
		bool add,
		FeatureVectorInt * ret) const {
	// bilingual features are defined in the target sentence
    for(int i = 0; i < (int)target.size(); i++){
		FeatureBilingual * feat_gen = dynamic_cast<FeatureBilingual*>(feature_gens_[i]);
		if (!feat_gen)
			THROW_ERROR("Only bilingual feature generator is acceptable")
		feat_gen->GenerateBilingualFeatures(*source[i], *target[i], align, edge, feature_ids, add, *ret);
	}
    return ret;
}

// Generates the biligual features that can be factored over a node
FeatureVectorInt * FeatureSet::MakeBilingualFeatures(
		const Sentence & source,
		const Sentence & target,
		const CombinedAlign & align,
		const HyperEdge & edge,
		SymbolSet<int> & feature_ids,
		bool add) const {
	return MakeBilingualFeatures(source, target, align, edge, feature_ids, add, new FeatureVectorInt);
}
Sentence FeatureSet::ParseInput(const string & line) const {
    vector<string> columns;
    algorithm::split(columns, line, is_any_of("\t"));
    if(feature_gens_.size() != columns.size()) {
        THROW_ERROR("Number of columns in feature file ("<<columns.size()<<
                    ") didn't equal feature profile ("<<feature_gens_.size()<<
                    ") at"<<endl<<line<<endl);
    }
    Sentence ret(columns.size());
    for(int i = 0; i < (int)columns.size(); i++) {
        ret[i] = feature_gens_[i]->ParseData(columns[i]);
    }
    return ret;
}

void FeatureSet::ParseConfiguration(const string & str) {
    config_str_ = str;
    if (str.length() == 0)
    	return;
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
    out << endl;
}
FeatureSet * FeatureSet::FromStream(istream & in) {
    string line;
    GetlineEquals(in, "feature_set");
    string config;
    GetConfigLine(in, "config_str", config);
    FeatureSet * ret = new FeatureSet;
    ret->ParseConfiguration(config);
    GetlineEquals(in, "");
    return ret;
}
