#ifndef FEATURE_BASE_H__ 
#define FEATURE_BASE_H__

#include <vector>
#include <string>
#include <lader/feature-vector.h>
#include <lader/feature-data-base.h>
#include <lader/hyper-edge.h>
#include <lader/dictionary.h>
#include <lader/symbol-set.h>

namespace lader {

// A virtual class for feature generators
// Four functions must be implemented to create a feature generator
//   ParseConfiguration, ParseData, GenerateEdgeFeatures
class FeatureBase {
public:
    // Types of features, whether they are factored over nodes, all edges,
    // or only nonterminal edges
    typedef enum {
        ALL_FACTORED,
        NONTERM_FACTORED
    } FeatureType;

    // A template for the features given the type, and a vector containing
    // the feature name and the values to be replaced
    typedef std::pair<FeatureType, std::vector<std::string> > FeatureTemplate;

    // Constructors
    FeatureBase() { }
    virtual ~FeatureBase() { }
 
    // Parses an arbitrary configuration std::string to decide the type of features
    // that the generator will construct
    virtual void ParseConfiguration(const std::string & str) = 0;

    // Parses an input std::string of data (that is in the appropriate format for
    // the feature generator) into the generator's internal representation
    virtual FeatureDataBase * ParseData(const std::string & str) = 0;

    // Generates the features that can be factored over an edge
    virtual void GenerateEdgeFeatures(
                                const FeatureDataBase & sentence,
                                const HyperEdge & edge,
                                SymbolSet<int> & feature_ids,
                                bool add,
                                FeatureVectorInt & feats) = 0;

    // Get the type string of this particular value
    virtual std::string GetType() const = 0;

    // Check to make sure this is equal to the right side
    virtual bool CheckEqual(const FeatureBase & rhs) const = 0;

    // Create a new sub-class of a particular type
    //  type=seq --> FeatureSequence
    static FeatureBase* CreateNew(const std::string & type);

    virtual bool FeatureTemplateIsLegal(const std::string & name) = 0;

    int GetBalance(const HyperEdge & edge);
    int GetSpanSize(const HyperEdge & edge);

protected:
    std::vector<FeatureTemplate> feature_templates_;

private:

};

}

#endif

