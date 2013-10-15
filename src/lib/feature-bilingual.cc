/*
 * feature-bilingual.cc
 *
 *  Created on: Oct 14, 2013
 *      Author: leona
 */
#include <sstream>
#include <cfloat>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/join.hpp>
#include <lader/discontinuous-hyper-edge.h>
#include <lader/hypothesis.h>

#include "feature-bilingual.h"
using namespace std;
using namespace boost;
using namespace lader;

bool FeatureBilingual::FeatureTemplateIsLegal(const string & name) {
	return FeatureSequence::FeatureTemplateIsLegal(name);
//			|| (name[0] == 'U'
//				&& (name.length() == 2
//					|| (name.length() == 3 && name[2] == '#'))); // unaligned word feature
}

int FeatureBilingual::GetMinF2E(char type, const HyperEdge & edge, const CombinedAlign & a)
{
    switch (type) {
	case 'S':
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			return std::min(a.GetMinF2E(edge.GetLeft(), e->GetM()),
					a.GetMinF2E(e->GetN(), e->GetRight()));
		}
		else
			return a.GetMinF2E(edge.GetLeft(), edge.GetRight());
	case 'L':
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				return a.GetMinF2E(edge.GetLeft(), e->GetM());
			// continuous + discontinuous = discontinuous
			else if (edge.GetCenter() <= e->GetM())
				return a.GetMinF2E(edge.GetLeft(), e->GetCenter()-1);
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() > e->GetN())
				return std::min(a.GetMinF2E(edge.GetLeft(), e->GetM()),
						a.GetMinF2E(e->GetN(), e->GetCenter()-1));
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				return std::min(a.GetMinF2E(edge.GetLeft(), e->GetM()-1),
						a.GetMinF2E(e->GetCenter(), e->GetN()-1));
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			return a.GetMinF2E(edge.GetLeft(), edge.GetCenter()-1);
		break;
	case 'R':
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				return a.GetMinF2E(e->GetN(), edge.GetRight());
			// continuous + discontinuous = discontinuous
			else if (edge.GetCenter() <= e->GetM())
				return std::min(a.GetMinF2E(edge.GetCenter(), e->GetM()),
						a.GetMinF2E(e->GetN(), e->GetRight()));
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() > e->GetN())
				return a.GetMinF2E(e->GetCenter(), edge.GetRight());
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				return std::min(a.GetMinF2E(e->GetM(), e->GetCenter()-1),
						a.GetMinF2E(e->GetN(), e->GetRight()));
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			return a.GetMinF2E(edge.GetCenter(), edge.GetRight());
		break;
	}
    return a.GetSrcLen();
}

int FeatureBilingual::GetMaxF2E(char type, const HyperEdge & edge, const CombinedAlign & a)
{
    switch (type) {
	case 'S':
		if (edge.GetClass() == 'D') {
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			return std::max(a.GetMaxF2E(edge.GetLeft(), e->GetM()),
					a.GetMaxF2E(e->GetN(), e->GetRight()));
		}
		else
			return a.GetMaxF2E(edge.GetLeft(), edge.GetRight());
	case 'L':
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				return a.GetMaxF2E(edge.GetLeft(), e->GetM());
			// continuous + discontinuous = discontinuous
			else if (edge.GetCenter() <= e->GetM())
				return a.GetMaxF2E(edge.GetLeft(), e->GetCenter()-1);
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() > e->GetN())
				return std::max(a.GetMaxF2E(edge.GetLeft(), e->GetM()),
						a.GetMaxF2E(e->GetN(), e->GetCenter()-1));
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				return std::max(a.GetMaxF2E(edge.GetLeft(), e->GetM()),
						a.GetMaxF2E(e->GetCenter(), e->GetN()-1));
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			return a.GetMaxF2E(edge.GetLeft(), edge.GetCenter()-1);
		break;
	case 'R':
		if (edge.GetClass() == 'D'){
			DiscontinuousHyperEdge * e = (DiscontinuousHyperEdge *)&edge;
			// continuous + continuous = discontinuous
			if (edge.GetCenter() < 0)
				return a.GetMaxF2E(e->GetN(), edge.GetRight());
			// continuous + discontinuous = discontinuous
			else if (edge.GetCenter() <= e->GetM())
				return std::max(a.GetMaxF2E(edge.GetCenter(), e->GetM()),
						a.GetMaxF2E(e->GetN(), e->GetRight()));
			// discontinuous + continuous = discontinuous
			else if (edge.GetCenter() > e->GetN())
				return a.GetMaxF2E(e->GetCenter(), edge.GetRight());
			// discontinuous + discontinuous = continuous
			else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN())
				return std::max(a.GetMaxF2E(e->GetM(), e->GetCenter()-1),
						a.GetMaxF2E(e->GetN(), e->GetRight()));
			else
				THROW_ERROR("Span is undefined for " << *((DiscontinuousHyperEdge*)e) << endl)
		}
		else
			return a.GetMaxF2E(edge.GetCenter(), edge.GetRight());
		break;
	}
    return -1;
}

string FeatureBilingual::GetFeatureString(const FeatureDataSequence & f,
											const FeatureDataSequence & e,
											const CombinedAlign & a,
                                             const HyperEdge & edge,
                                             const std::string & str){
	ostringstream oss;
	if (str[0] == 'C'){
		int lsize = std::max(0, GetMaxF2E('L', edge, a) - GetMinF2E('L', edge, a) + 1);
		int rsize = std::max(0, GetMaxF2E('R', edge, a) - GetMinF2E('R', edge, a) + 1);
		switch(str[1]){
		// Get the difference between values
		case 'D':
			oss << abs(rsize - lsize);
			return oss.str();
		case 'B':
			oss << rsize - lsize;
			return oss.str();
		case 'L':
			if (lsize > rsize )
				return "L";
			else if (lsize < rsize)
				return "R";
			return "E";
		default:
			THROW_ERROR("Bad compare feature type " << str[1])
			break;
		}
	}
	else if (str[0] == 'E' && str[1] == 'T'){
		oss << (char)edge.GetType() << edge.GetClass();
		return oss.str();
	}
	// Features defined over a span
    int l = GetMinF2E(str[0], edge, a);
    int r = GetMaxF2E(str[0], edge, a);
	if (l >= e.GetNumWords() || r < 0)
		return "NULL";
	switch (str[1]) {
	case 'L':
		return e.GetElement(l);
	case 'R':
		return e.GetElement(r);
	case 'B':
		return l == 0 ? "<s>" : e.GetElement(l-1);
	case 'A':
		return r == e.GetNumWords() - 1 ? "</s>" : e.GetElement(r+1);
	case 'S':
		return e.GetRangeString(l, r, (str.length() == 3 ? str[2]-'0' : INT_MAX));
	case 'Q':
		return SafeAccess(dicts_,
				str[3]-'0')->Exists(e.GetRangeString(l, r)) ?
						"+" : "-";
	case 'N':
		oss << r - l + 1;
		return oss.str();
	default:
		THROW_ERROR("Bad edge feature type " << str[1])
		break;
	}
	return "";
}

double FeatureBilingual::GetFeatureValue(const FeatureDataSequence & f,
											const FeatureDataSequence & e,
											const CombinedAlign & a,
                                             const HyperEdge & edge,
                                             const std::string & str) {
	if (str[0] == 'C'){
		int lsize = std::max(0, GetMaxF2E('L', edge, a) - GetMinF2E('L', edge, a) + 1);
		int rsize = std::max(0, GetMaxF2E('R', edge, a) - GetMinF2E('R', edge, a) + 1);
		switch(str[1]){
		// Get the difference between values
		case 'D':
			return abs(rsize - lsize);
		case 'B':
			return rsize - lsize;
		default:
			THROW_ERROR("Bad compare feature type " << str[1])
			break;
		}
	}
	int l = GetMinF2E(str[0], edge, a);
	int r = GetMaxF2E(str[0], edge, a);
	if (l >= e.GetNumWords() || r < 0)
		return -DBL_MAX;
	switch (str[1]) {
	case 'N':
		return r - l + 1;
	case 'Q':
		return SafeAccess(dicts_,str[3]-'0')->GetFeature(
				e.GetRangeString(l, r), str[4]-'0');
	default:
		THROW_ERROR("Bad feature value " << str[1]);
		break;
    }
    return -DBL_MAX;
}

// Generates the features that can be factored over a hypothesis
void FeatureBilingual::GenerateBilingualFeatures(
							const FeatureDataBase & source,
							const FeatureDataBase & target,
							const CombinedAlign & align,
							const HyperEdge & edge,
                            SymbolSet<int> & feature_ids,
                            bool add,
                            FeatureVectorInt & feats){
    const FeatureDataSequence & f = (const FeatureDataSequence &)source;
    const FeatureDataSequence & e = (const FeatureDataSequence &)target;
    bool is_nonterm = (edge.GetType() == HyperEdge::EDGE_INV ||
                           edge.GetType() == HyperEdge::EDGE_STR);
    // Iterate over each feature
	BOOST_FOREACH(FeatureTemplate templ, feature_templates_) {
		if (templ.first == ALL_FACTORED || is_nonterm) {
			ostringstream values; values << templ.second[0];
			double feat_val = 1;
			// templ.second is the vector of the feature templates
			for(int i = 1; i < (int)templ.second.size(); i++) {
				if(templ.second[i].length() >= 3 && templ.second[i][2] == '#')
					feat_val = GetFeatureValue(f, e, align, edge, templ.second[i]);
				else
					values << "||" << GetFeatureString(f, e, align, edge, templ.second[i]);
			}
			if (feat_val){
				int id = feature_ids.GetId(values.str(), add);
				if (id >= 0)
					feats.push_back(MakePair(id, feat_val));
			}
		}
	}
}
