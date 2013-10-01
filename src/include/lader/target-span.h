#ifndef TARGET_SPAN_H__ 
#define TARGET_SPAN_H__

#include <boost/foreach.hpp>
#include <lader/hypothesis.h>
#include <lader/symbol-set.h>
#include <lader/util.h>
#include <vector>
#include <cfloat>
#include <set>
#include <sstream>
#include <iostream>
#include <stdio.h>

using namespace std;

namespace lader {

class HyperGraph;

class TargetSpan {
public:
    TargetSpan(int left, int right) :
			left_(left), right_(right), id_(-1) {
		// resize the feature list in advance
    	if (right - left == 0){
    		straight.resize(1, NULL);
    		inverted.resize(1, NULL);
    	}
    	else{
    		straight.resize(right - left + 2, NULL);
    		inverted.resize(right - left + 2, NULL);
    	}
	}

    void ClearHypotheses()
    {
        BOOST_FOREACH(Hypothesis * hyp, hyps_)
            delete hyp;
        hyps_.clear();
        while(!cands_.empty()){
            delete cands_.top();
            cands_.pop();
        }
    }

    virtual ~TargetSpan()
    {
        ClearHypotheses();
        BOOST_FOREACH(FeatureVectorInt* fvi, straight)
        if (fvi)
        	delete fvi;
        BOOST_FOREACH(FeatureVectorInt* fvi, inverted)
        if (fvi)
        	delete fvi;
    }

    void LabelWithIds(int & curr_id){
    	if(id_ != -1) return;
    	BOOST_FOREACH(Hypothesis * hyp, hyps_) {
    		if(hyp->GetLeftChild())
    			hyp->GetLeftChild()->LabelWithIds(curr_id);
    		if(hyp->GetRightChild())
    			hyp->GetRightChild()->LabelWithIds(curr_id);
    	}
    	id_ = curr_id++;
    }

    int GetId() const { return id_; }
    int GetLeft() const { return left_; }
    int GetRight() const { return right_; }
    // Add a hypothesis with a new hyper-edge
    void AddHypothesis(Hypothesis * hyp) { hyps_.push_back(hyp); }
    // Add an unique hypothesis with a new hyper-edge
    bool AddUniqueHypothesis(Hypothesis * hyp) {
    	BOOST_FOREACH(Hypothesis * prev, hyps_)
			if (*prev == *hyp)
				return false;
    	hyps_.push_back(hyp);
    	return true;
    }
    const std::set<char> & GetHasTypes() { return has_types_; }
    const std::vector<Hypothesis*> & GetHypotheses() const { return hyps_; }
    std::vector<Hypothesis*> & GetHypotheses() { return hyps_; }
    const Hypothesis* GetHypothesis(int i) const { return SafeAccess(hyps_,i); }
    Hypothesis* GetHypothesis(int i) {
        if((int)hyps_.size() <= i)
            return NULL;
        else
        	return hyps_[i];
    }
    HypothesisQueue & GetCands() { return cands_; }
    size_t HypSize() const { return hyps_.size(); }
    size_t CandSize() const { return cands_.size(); }
    const Hypothesis* operator[] (size_t val) const { return hyps_[val]; }
    Hypothesis* operator[] (size_t val) { return hyps_[val]; }
    void ResetId() { id_ = -1; has_types_.clear(); }
    void SetId(int id) { id_ = id; }
    void SetHasType(char c) { has_types_.insert(c); }
    void SaveStraightFeautures(int c, FeatureVectorInt * fvi) {
    	if (straight[c < 0? 0 : c])
    		THROW_ERROR("Features are already saved")
		if (c < 0)
			straight[0] = fvi;
		else
			straight[c] = fvi;
	}
	void SaveInvertedFeautures(int c, FeatureVectorInt * fvi) {
		if (inverted[c < 0? 0 : c])
			THROW_ERROR("Features are already saved")
		if (c < 0)
			inverted[0] = fvi;
		else
			inverted[c] = fvi;
	}
	FeatureVectorInt * GetStraightFeautures(int c) {
		if (c < 0)
			return straight[0];
		else
			return straight[c];
	}
	FeatureVectorInt * GetInvertedFeautures(int c) {
		if (c < 0)
			return inverted[0];
		else
			return inverted[c];
	}
    void WriteFeatures(std::ostream & out)
    {
        int n = left_ == right_ ? 1 : right_ - left_ + 2;
        for (int c = 0 ; c < n ; c++){
			int i = 0;
			if (straight[c])
				BOOST_FOREACH(FeaturePairInt & feat, *(straight[c])){
					if (i++ != 0) out << " ";
					out << feat.first << ":" << feat.second;
				}
			out << endl;
		}
        for (int c = 0 ; c < n ; c++){
			int i = 0;
			if (inverted[c])
				BOOST_FOREACH(FeaturePairInt & feat, *(inverted[c])){
					if (i++ != 0) out << " ";
					out << feat.first << ":" << feat.second;
				}
			out << endl;
		}
    }

	void ReadFeatures(std::istream & in)
    {
        int n = left_ == right_ ? 1 : right_ - left_ + 2;
        for (int c = 0 ; c < n ; c++){
			std::string line;
			std::getline(in, line);
			if (line.empty()){
				SaveStraightFeautures(c, NULL);
				continue;
			}
			FeatureVectorInt * fvi = new FeatureVectorInt;
			int id;
			float value;
			const char * data = line.c_str();
			while(sscanf(data, "%d:%f", &id, &value) == 2){
				fvi->push_back(MakePair(id, value));
				while(!(*data == ' ' || *data == '\0'))
					data++;
				if (*data == ' ')
					data++;
				else
					break;
			}
			SaveStraightFeautures(c, fvi);
		}
        for (int c = 0 ; c < n ; c++){
			std::string line;
			std::getline(in, line);
			if (line.empty()){
				SaveInvertedFeautures(c, NULL);
				continue;
			}
			FeatureVectorInt * fvi = new FeatureVectorInt;
			int id;
			float value;
			const char * data = line.c_str();
			while(sscanf(data, "%d:%f", &id, &value) == 2){
				fvi->push_back(MakePair(id, value));
				while(!(*data == ' ' || *data == '\0'))
					data++;
				if (*data == ' ')
					data++;
				else
					break;
			}
			SaveInvertedFeautures(c, fvi);
		}
    }


    // IO Functions for stored features
    virtual void FeaturesToStream(std::ostream & out)
    {
        out << "[" << left_ << ", " << right_ << "]" << endl;
        WriteFeatures(out);
	}

    virtual void FeaturesFromStream(std::istream & in)
    {
        std::string line;
        std::stringstream header;
        header << "[" << left_ << ", " << right_ << "]";
        GetlineEquals(in, header.str());
        ReadFeatures(in);
	}
protected:
    std::vector<Hypothesis*> hyps_;
    // For cube growing
    HypothesisQueue cands_;
    // Store edge features
    // There are two possible orientations
    // Each list of FeatureVectorInts contains
	// list[0] is the terminal node (forward/backward)
	// list[1:] are the non-terminal nodes (straight/inverted)
    // It is possible to have NULL elements
    std::vector<FeatureVectorInt*> straight;
    std::vector<FeatureVectorInt*> inverted;
    int left_, right_;

private:
    int id_;
    std::set<char> has_types_;
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator << ( std::ostream& out,
                                   const lader::TargetSpan & rhs )
{
    out << "[" << rhs.GetLeft() << ", " << rhs.GetRight() << "]";
    return out;
}
}

#endif

