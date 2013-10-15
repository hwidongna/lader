#include <lader/feature-sequence.h>

#include <sstream>
#include <cfloat>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/join.hpp>
#include <lader/discontinuous-hyper-edge.h>

using namespace boost;
using namespace lader;
using namespace std;

bool FeatureSequence::FeatureTemplateIsLegal(const string & name) {
    if(name.length() < 2)
        return false;
    // For edge values, the only type is 'T'
    if(name[0] == 'E') {
        return name[1] == 'T';
    // For comparison values, difference, balance, or longer side
    } else if(name[0] == 'C') {
        return name[1] == 'D' || name[1] == 'B' || name[1] == 'L';
    } else {
        // For sequence matcher Q, check to make sure that the type
        //  name[2] = E (existance indicator) or # (feature)
        //  name[3] = [0-9] (for the number of the dictionary)
        //  name[4] = [0-9] (for the number of the feature if #)
        if(name[1] == 'Q') {
            return ((name.length() == 5 && name[2] == '#' 
                            && IsDigit(name[3]) && IsDigit(name[4])) || 
                   (name.length() == 4 && name[2] == 'E' && IsDigit(name[3])));
        // For other span values
        } else if(name[1] == 'S') {
            return name.length() == 2 || 
                (name.length() == 3 && 
                    (name[2] == '#' || (name[2] >= '0' && name[2] <= '9')));
        // For random values
        } else if(name[1] == 'M') {
            return name.length() == 3 && name[2] == '#';
        // For other values
        } else if (name[1] == 'N' || name[1] == 'L' || name[1] == 'R'
                                  || name[1] == 'B' || name[1] == 'A') {
            return name.length() == 2 || (name.length() == 3 && name[2] == '#');
        } else {
            return false;
        }
    }
}

// Parse the comma-separated list of configuration options
void FeatureSequence::ParseConfiguration(const string & str) {
    feature_templates_.clear();
    // Iterate through all the comma-separated strings
    tokenizer<char_separator<char> > feats(str, char_separator<char>(","));
    BOOST_FOREACH(string feat, feats) {
        // First check if this is specifying a special option
        if(!feat.compare(0, 5, "dict=")) {
            ifstream in(feat.substr(5).c_str());
            if(!in)
                THROW_ERROR("Could not find dictionary: " << feat.substr(5).c_str());
            dicts_.push_back(Dictionary::FromStream(in));
            continue;
        }
        // Otherwise assume a feature template
        FeatureType my_type = ALL_FACTORED;
        vector<string> my_name;
        // Iterate through all the percent-separated features (first is name)
        tokenizer<char_separator<char> > 
                        items(feat, char_separator<char>("%"));
        BOOST_FOREACH(string item, items) {
            if(my_name.size()) {
                if(!FeatureTemplateIsLegal(item))
                    THROW_ERROR("Illegal feature template "<<
                                item<<" in "<<str);
                // If left span, right span, or comparison are necessary,
                // this feature is only applicable to non-terminals
                if(item[0] == 'L' || item[0] == 'R' || item[0] == 'C')
                    my_type = NONTERM_FACTORED;
            }
            my_name.push_back(item);
        }
        feature_templates_.push_back(MakePair(my_type, my_name));
    }
}

// Parses an input string of data (that is in the appropriate format for
// the feature generator) into the generator's internal representation
FeatureDataBase * FeatureSequence::ParseData(const string & str) {
    FeatureDataSequence * data = new FeatureDataSequence;
    data->FromString(str);
    return data;
}

string FeatureSequence::GetFeatureString(const FeatureDataSequence & sent,
                                             const HyperEdge & edge,
                                             const std::string & str) {
	int l = 0, r = -1;
    ostringstream oss;
    int bal;
	switch (str[0]) {
	case 'S':
		if (str[1] == 'N') {// for discontinuous hyper-edge
			oss << GetSpanSize(edge);
			return oss.str();
		}
		l = edge.GetLeft();
		r = edge.GetRight();
		break;
	case 'L':
		l = edge.GetLeft();
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				r = e->GetM();
			// continuous + discontinuous = discontinuous
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() <= e->GetM() || edge.GetCenter() > e->GetN())
				r = edge.GetCenter()-1;
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				r = e->GetN()-1;
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			r = edge.GetCenter()-1;
		break;
	case 'R':
		r = edge.GetRight();
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				l = e->GetN();
			// continuous + discontinuous = discontinuous
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() <= e->GetM() || edge.GetCenter() > e->GetN())
				l = edge.GetCenter();
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				l = e->GetM();
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			l = edge.GetCenter();
		break;
	case 'C': // compare type
		switch (str[1]) {
		// Get the difference between values
		case 'D':
			oss << abs(GetBalance(edge));
			return oss.str();
		case 'B':
		case 'L':
			// Get the balance between the values
			bal = GetBalance(edge);
			if(str[1] == 'B') { oss << bal; return oss.str(); }
			else if(bal < 0) { return "L"; }
			else if(bal > 0) { return "R"; }
			else { return "E"; }
		default:
			THROW_ERROR("Bad compare feature type " << str[1])
			break;
	    }
		break;
	}
//    if ( l > r ) THROW_ERROR("Invalid span ["<<l<<", "<<r<<"]")
	switch (str[1]) {
	case 'L':
		return sent.GetElement(l);
	case 'R':
		return sent.GetElement(r);
	case 'B':
		return l == 0 ? "<s>" : sent.GetElement(l-1);
	case 'A':
		return r == sent.GetNumWords() - 1 ? "</s>" : sent.GetElement(r+1);
	case 'S':
		return sent.GetRangeString(l, r, (str.length() == 3 ? str[2]-'0' : INT_MAX));
	case 'Q':
		return SafeAccess(dicts_,
				str[3]-'0')->Exists(sent.GetRangeString(l, r)) ?
						"+" : "-";
	case 'N':
		oss << r - l + 1;
		return oss.str();
	case 'T':
		oss << (char)edge.GetType() << edge.GetClass();
		return oss.str();
	default:
		THROW_ERROR("Bad edge feature type " << str[1])
		break;
	}
    return "";
}

double FeatureSequence::GetFeatureValue(const FeatureDataSequence & sent,
                                             const HyperEdge & edge,
                                             const std::string & str) {
	int l = 0, r = -1;
	switch (str[0]) {
	case 'S':
		if (str[1] == 'N') // for discontinuous hyper-edge
			return GetSpanSize(edge);
		l = edge.GetLeft();
		r = edge.GetRight();
		break;
	case 'L':
		l = edge.GetLeft();
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				r = e->GetM();
			// continuous + discontinuous = discontinuous
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() <= e->GetM() || edge.GetCenter() > e->GetN())
				r = edge.GetCenter()-1;
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				r = e->GetN()-1;
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			r = edge.GetCenter()-1;
		break;
	case 'R':
		r = edge.GetRight();
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				l = e->GetN();
			// continuous + discontinuous = discontinuous
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() <= e->GetM() || edge.GetCenter() > e->GetN())
				l = edge.GetCenter();
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				l = e->GetM();
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			l = edge.GetCenter();
		break;
	case 'C': // compare type
		switch (str[1]) {
		// Get the difference between values
		case 'D':
			return abs(GetBalance(edge));
		case 'B':
			return GetBalance(edge);
		default:
			THROW_ERROR("Bad compare feature type " << str[1])
			break;
		}
		break;
	}
	if ( l > r ) THROW_ERROR("Invalid span ["<<l<<", "<<r<<"]")
    char type = str[1];
	switch (type) {
	case 'N':
		return r - l + 1;
	case 'Q':
		return SafeAccess(dicts_,str[3]-'0')->GetFeature(
				sent.GetRangeString(l, r), str[4]-'0');
		// Get the difference between values
	default:
		THROW_ERROR("Bad feature value " << type);
    }
    return -DBL_MAX;
}

// Generates the features that can be factored over an edge
void FeatureSequence::GenerateEdgeFeatures(
                            const FeatureDataBase & sent,
                            const HyperEdge & edge,
                            SymbolSet<int> & feature_ids,
                            bool add,
                            FeatureVectorInt & feat) {
    const FeatureDataSequence & sent_seq = (const FeatureDataSequence &)sent;
    bool is_nonterm = (edge.GetType() == HyperEdge::EDGE_INV || 
                       edge.GetType() == HyperEdge::EDGE_STR);
    // Iterate over each feature
    BOOST_FOREACH(FeatureTemplate templ, feature_templates_) {
        // Make sure that this feature is compatible with the edge
    	// templ.first is the type
        if (templ.first == ALL_FACTORED || is_nonterm) {
            ostringstream values; values << templ.second[0];
            double feat_val = 1;
            // templ.second is the vector of the feature templates
            for(int i = 1; i < (int)templ.second.size(); i++) {
                if(templ.second[i].length() >= 3 && templ.second[i][2] == '#')
                	feat_val = GetFeatureValue(sent_seq, edge, templ.second[i]);
                else
                	values << "||" << GetFeatureString(sent_seq, edge, templ.second[i]);
            }
            if(feat_val) {
                int id = feature_ids.GetId(values.str(), add);
                if(id >= 0)
                    feat.push_back(MakePair(id,feat_val));
            }
        }
    }
}
