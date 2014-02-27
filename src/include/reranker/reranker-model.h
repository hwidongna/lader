/*
 * reranker-model.h
 *
 *  Created on: Feb 24, 2014
 *      Author: leona
 */

#ifndef RERANKER_MODEL_H_
#define RERANKER_MODEL_H_

#include <vector>
#include <lader/feature-set.h>
#include <lader/feature-vector.h>
#include <boost/thread/mutex.hpp>

using namespace lader;

namespace reranker {

// A reorderer model that contains the weights and the feature set
class RerankerModel {
	typedef std::tr1::unordered_map<int,int> ConversionTable;
public:

    // Initialize the reranker model
    RerankerModel()
        : t_(1), alpha_(1), v_squared_norm_(0), lambda_(1e-5), add_features_(true) { }

    // Calculate the score of a feature vector
    double ScoreFeatureVector(const FeatureVectorInt & vec) const {
        double ret = 0;
        BOOST_FOREACH(const FeaturePairInt & fpi, vec)
            ret += GetWeight(fpi.first) * fpi.second;
        return ret;
    }

    // Get the feature IDs
    SymbolSet<int> & GetFeatureIds() {
        return feature_ids_;
    }
    const SymbolSet<int> & GetFeatureIds() const {
        return feature_ids_;
    }

    // Get an indexed feature vector from a string feature vector
    FeatureVectorInt * IndexFeatureVector(const FeatureVectorString & str) {
        FeatureVectorInt * ret = new FeatureVectorInt(str.size());
        for(int i = 0; i < (int)str.size(); i++)
            (*ret)[i] = MakePair(feature_ids_.GetId(str[i].first,add_features_),
                                 str[i].second);
        return ret;
    }
    // Get the strings for a feature vector index
    FeatureVectorString *StringifyFeatureVector(const FeatureVectorInt & str){
        FeatureVectorString * ret = new FeatureVectorString(str.size());
        for(int i = 0; i < (int)str.size(); i++)
            (*ret)[i] = MakePair(feature_ids_.GetSymbol(str[i].first),
                                 str[i].second);
        return ret;
    }

    // IO Functions
    void ToStream(std::ostream & out, int thresold = 5);
    static RerankerModel * FromStream(std::istream & in, bool renumber=false);

    // Comparators
    bool operator==(const RerankerModel & rhs) const {
        BOOST_FOREACH(const std::string * str, feature_ids_.GetSymbols())
            if(GetWeight(*str) != rhs.GetWeight(*str))
                return false;
        BOOST_FOREACH(const std::string * str, rhs.feature_ids_.GetSymbols())
            if(GetWeight(*str) != rhs.GetWeight(*str))
                return false;
        return true;
    }
    bool operator!=(const RerankerModel & rhs) const {
        return !(*this == rhs);
    }

    // Accessors
    double GetWeight(int id) const {
        return id >= 0 && id < (int)v_.size() ? v_[id] * alpha_: 0;
    }
    double GetWeight(const std::string & str) const {
        return GetWeight(feature_ids_.GetId(str));
    }
    // Get the weight array
    const std::vector<double> & GetWeights() {
        // Update the values for the entire array
        if(alpha_ != 1) {
            BOOST_FOREACH(double & val, v_)
                val *= alpha_;
            v_squared_norm_ *= alpha_*alpha_;
            alpha_ = 1;
        }
        return v_;
    }
    // Set the weight appropriately
    void SetWeight(const std::string & name, double w) {
        int idx = feature_ids_.GetId(name, true);
        if((int)v_.size() <= idx)
            v_.resize(idx+1,0.0);
        double old_val = v_[idx];
        v_[idx] = w/alpha_;
        v_squared_norm_ += v_[idx]*v_[idx] - old_val*old_val;
    }
    int GetCount(int id){
    	return id >= 0 && id < (int)counts_.size() ? counts_[id]: 0;
    }
    // Set the count appropriately
    void SetCount(int idx, const std::string & name, int c, bool renumber) {
    	if (renumber){
    		int newidx = feature_ids_.GetId(name, true);
    		ConversionTable::iterator it = conversion_.find(idx);
    		if (it != conversion_.end())
    			THROW_ERROR(idx << " already indicates " << it->second);
    		conversion_[idx] = newidx;
    		idx = newidx;
    	}
    	else
    		feature_ids_.SetId(idx, name);
    	if((int)counts_.size() <= idx)
    		counts_.resize(idx+1, 0);
    	counts_[idx] = c;
    }
    void SetCost(double cost) { lambda_ = cost; }
    void SetAdd(bool add) { add_features_ = add; }
    bool GetAdd() const { return add_features_; }

    int GetId(const std::string & sym, bool add = false){
    	int id = feature_ids_.GetId(sym, add);
    	if (add){
    		boost::mutex::scoped_lock lock(mutex_);
    		if (counts_.size() <= id)
    			counts_.resize(id+1, 0);
    		counts_[id]++;
    	}
    	return id;
    }

    int GetConvertion(int idx){
    	ConversionTable::iterator it = conversion_.find(idx);
    	if (it == conversion_.end())
    		return -1;
    	return it->second;
    }
private:
    boost::mutex mutex_;
    // The number of times for each feature appears
    std::vector<int> counts_;
    // Weights over features and weights over losses
    std::vector<double> v_;
    // Parameters for the pegasos algorithm
    int t_;
    // The scaling factor, squared norm, and cost used in learning Pegasos
    double alpha_, v_squared_norm_, lambda_;
    // Feature name values
    SymbolSet<int> feature_ids_; // Feature names and IDs
    bool add_features_; // Whether to allow the adding of new features
    ConversionTable conversion_; // Before and After conversion
};

}



#endif /* RERANKER_MODEL_H_ */
