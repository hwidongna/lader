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

// A reranker model that contains the weights and the feature count
class RerankerModel {
	typedef std::tr1::unordered_map<int,int> ConversionTable;
public:

    // Initialize the reranker model
    RerankerModel() : add_features_(true), max_swap_(0) { }

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

    // IO Functions
    void ToStream(std::ostream & out, int thresold = 5);
    static RerankerModel * FromStream(std::istream & in, bool renumber=false);
    void SetWeightsFromStream(std::istream & in);

    // Accessors
    double GetWeight(int id) const {
        return id >= 0 && id < (int)v_.size() ? v_[id] : 0;
    }
    double GetWeight(const std::string & str) const {
        return GetWeight(feature_ids_.GetId(str));
    }
    // Get the weight array
    const std::vector<double> & GetWeights() { return v_; }
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
    void SetAdd(bool add) { add_features_ = add; }
    bool GetAdd() const { return add_features_; }
    void SetMaxSwap(int max_swap) { max_swap_ = max_swap; }
    int GetMaxSwap() { return max_swap_; }

    int GetId(const std::string & sym, bool add = false){
    	int id = feature_ids_.GetId(sym, add);
    	if (add){
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

    double GetScore(const FeatureMapInt & featmap){
    	double result = 0;
    	BOOST_FOREACH(FeaturePairInt fp, featmap){
    		result += GetWeight(fp.first) * fp.second;
    	}
    	return result;
    }
private:
    // the maximum number of swap actions
    int max_swap_;
    // The number of times for each feature appears
    std::vector<int> counts_;
    // Weights over features and weights over losses
    std::vector<double> v_;
    // Feature name values
    SymbolSet<int> feature_ids_; // Feature names and IDs
    bool add_features_; // Whether to allow the adding of new features
    ConversionTable conversion_; // Before and After conversion
};

}



#endif /* RERANKER_MODEL_H_ */
