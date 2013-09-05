#include <lader/feature-non-local.h>

#include <sstream>
#include <cfloat>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/join.hpp>
#include <lader/discontinuous-hyper-edge.h>
#include <lader/hypothesis.h>

using namespace boost;
using namespace lader;
using namespace std;

bool FeatureNonLocal::FeatureTemplateIsLegal(const string & name) {
    if(name.length() < 2)
        return false;
    if(name[0] == 'S' || name[0] == 'L' || name[0] == 'R')
        return name[1] == 'L' || name[1] == 'R';
//    else if(name[0] == 'B')
//    	return name[1] == 'E' || name[1] == 'P';
    return false;
}

// Parse the comma-separated list of configuration options
void FeatureNonLocal::ParseConfiguration(const string & str) {
    feature_templates_.clear();
    // Iterate through all the comma-separated strings
    tokenizer<char_separator<char> > feats(str, char_separator<char>(","));
    BOOST_FOREACH(string feat, feats) {
    	// non-local feature is only applicable to non-terminals
        FeatureType my_type = NONTERM_FACTORED;
        vector<string> my_name;
        // Iterate through all the percent-separated features (first is name)
        tokenizer<char_separator<char> > 
                        items(feat, char_separator<char>("%"));
        BOOST_FOREACH(string item, items) {
            if(my_name.size()) {
                if(!FeatureTemplateIsLegal(item))
                    THROW_ERROR("Illegal feature template "<<
                                item<<" in "<<str);
            }
            my_name.push_back(item);
        }
        feature_templates_.push_back(MakePair(my_type, my_name));
    }
}

// Parses an input string of data (that is in the appropriate format for
// the feature generator) into the generator's internal representation
FeatureDataBase * FeatureNonLocal::ParseData(const string & str) {
    FeatureDataSequence * data = new FeatureDataSequence;
    data->FromString(str);
    return data;
}

string FeatureNonLocal::GetNonLocalFeatureString(const FeatureDataSequence & sent,
                                             int l, int r,
                                             const string & str) {
    ostringstream oss;
    char type = str[1];
    lm::WordIndex lword, rword;
    lm::FullScoreReturn ret;
    switch (type) {
        case 'L':
            return sent.GetElement(l);
        case 'R':
            return sent.GetElement(r);
//        case 'E':
//        	// TODO: qunatize probability
//        case 'P':
//        	if (!bigram_)
//        		THROW_ERROR("Load language model first");
//        	lword = bigram_->GetVocabulary().Index(sent.GetElement(l));
//        	rword = bigram_->GetVocabulary().Index(sent.GetElement(r));
//        	State out_state;
//        	ret = bigram_->FullScoreForgotState(&lword, &lword, rword, out_state);
//            oss << ret.prob;
//            return oss.str();
        default:
            THROW_ERROR("Bad span feature type " << type);
    }
    return "";
}

// Generates the features that can be factored over a hypothesis
void FeatureNonLocal::GenerateNonLocalFeatures(
                            const FeatureDataBase & sent,
                            const Hypothesis & left,
                    	    const Hypothesis & right,
                            SymbolSet<int> & feature_ids,
                            bool add,
                            FeatureVectorInt & feats){
    const FeatureDataSequence & sent_seq = (const FeatureDataSequence &)sent;
    // Iterate over each feature
	BOOST_FOREACH(FeatureTemplate templ, feature_templates_) {
		ostringstream values; values << templ.second[0];
		double feat_val = 1;
        // templ.second is the vector of the feature templates
        for(int i = 1; i < (int)templ.second.size(); i++) {
        	switch (templ.second[i][0]) {
				case 'S':
					values << "||" << GetNonLocalFeatureString(sent_seq,
							left.GetTrgLeft(), right.GetTrgRight(), templ.second[i]);
					break;
				case 'L':
					values << "||" << GetNonLocalFeatureString(sent_seq,
							left.GetTrgLeft(), left.GetTrgRight(), templ.second[i]);
					break;
				case 'R':
					values << "||" << GetNonLocalFeatureString(sent_seq,
							right.GetTrgLeft(), right.GetTrgRight(), templ.second[i]);
					break;
//				case 'B':
//					values << "||" << GetNonLocalFeatureString(sent_seq,
//							left.GetTrgRight(), right.GetTrgLeft(), templ.second[i]);
//					break;
			}
        }
        if (feat_val){
        	int id = feature_ids.GetId(values.str(), add);
        	if (id >= 0)
        		feats.push_back(MakePair(id, feat_val));
        }
	}
}
