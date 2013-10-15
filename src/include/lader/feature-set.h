#ifndef FEATURE_SET_H__ 
#define FEATURE_SET_H__

#include <lader/feature-data-base.h>
#include <lader/feature-base.h>
#include <lader/symbol-set.h>
#include <lader/combined-alignment.h>
#include <boost/foreach.hpp>

namespace lader {

class Hypothesis;
// A class containing a set of features defined over various data types
// Can assign features to the nodes and edges of a hypergraph
class FeatureSet {
public:

    FeatureSet() : max_term_(0), use_reverse_(true) { }
    ~FeatureSet() {
        BOOST_FOREACH(FeatureBase * gen, feature_gens_)
            if(gen)
                delete gen;
    }

    // Add a feature generator, and take control of it
    void AddFeatureGenerator(FeatureBase * gen) {
        feature_gens_.push_back(gen);
    }

    // Generates the features that can be factored over a node
    FeatureVectorInt * MakeEdgeFeatures(
        const Sentence & sent,
        const HyperEdge & edge,
        SymbolSet<int> & feature_ids,
        bool add_features) const;
    
    // Generates the biligual features that can be factored over a node
    // This is useful to accumulate features to the existing vector (ret)
    FeatureVectorInt *MakeBilingualFeatures(const Sentence & source,
    			const Sentence & target, const CombinedAlign & align,
    			const HyperEdge & edge, SymbolSet<int> & feature_ids, bool add,
    			FeatureVectorInt * ret) const;

    // Generates the biligual features that can be factored over a node
    FeatureVectorInt * MakeBilingualFeatures(
		const Sentence & source,
		const Sentence & target,
		const CombinedAlign & align,
		const HyperEdge & edge,
		SymbolSet<int> & feature_ids,
		bool add_features) const;

    // Change an integer-indexed feature vector into a string-indexed vector
    FeatureVectorString StringifyFeatureIndices(const FeatureVectorInt & vec);

    // Parse a multi-factor input separated by tabs
    Sentence ParseInput(const std::string & line) const;

    // Parse the configuration
    void ParseConfiguration(const std::string & str);

    // IO Functions
    void ToStream(std::ostream & out);
    static FeatureSet * FromStream(std::istream & in);

    // Comparators
    bool operator== (const FeatureSet & rhs) {
        return (config_str_ == rhs.config_str_ &&
                feature_gens_.size() == rhs.feature_gens_.size() &&
                // feature_ids_->size() == rhs.feature_ids_->size() &&
                max_term_ == rhs.max_term_);
    }

    // Accessors
    const FeatureBase* GetGenerator(int id) const { return feature_gens_[id]; }
    int GetMaxTerm() const { return max_term_; }
    bool GetUseReverse() const { return use_reverse_; }

    void SetMaxTerm(int max_term) { max_term_ = max_term; }
    void SetUseReverse(bool use_reverse) { use_reverse_ = use_reverse; }

private:

    std::string config_str_; // The configuration string
    int max_term_; // The maximum length of a terminal
    std::vector<FeatureBase*> feature_gens_; // Feature generators
    bool use_reverse_; // Reverse

};

}

#endif

