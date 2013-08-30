#ifndef FEATURE_PARSE_H__ 
#define FEATURE_PARSE_H__

#include <vector>
#include <string>
#include <lader/feature-data-parse.h>
#include <lader/feature-base.h>
#include <lader/dictionary.h>

namespace lader {

// A class to calculate features that concern parses of words, tags, etc.
class FeatureParse : public FeatureBase {
public:

    FeatureParse() { }
    virtual ~FeatureParse() { }
 
    // Parse the configuration
    // The configuration takes the following format:
    //  NAME%X%Y where NAME is the overall name and X and Y are template
    // 
    // The templates that can be used will all start with:
    //  S: For features designating an entire span
    //  L or R: For features defined over the left or right spans
    //  C: For features that compare the two spans
    //  E: For features defined over an edge (edge type)
    //
    // The templates that can be used and combined for binary features are:
    //  %[SLR]P :    Span label if exists, X otherwise
    //  %CD :        Difference (absolute value) in words in two spans
    //  %ET :        The type of the edge
    // 
    // In addition N and D can be used with at the beginning (eg %SN#) to
    // fire non-binary features. These cannot be combined with other features
    virtual void ParseConfiguration(const std::string & str);

    // Parses a space-separated input string of data
    virtual FeatureDataBase * ParseData(const std::string & str);

    // Generates the features that can be factored over an edge
    virtual void GenerateEdgeFeatures(
                                const FeatureDataBase & sentence,
                                const HyperEdge & edge,
                                SymbolSet<int> & feature_ids,
                                bool add,
                                FeatureVectorInt & feats);

    // Get the type of this feature generator
    virtual std::string GetType() const { return "parse"; }

    // Check whether this is equal
    virtual bool CheckEqual(const FeatureBase & rhs) const {
        if(rhs.GetType() != this->GetType())
            return false;
        const FeatureParse & rhs_seq = (const FeatureParse &)rhs;
        if(feature_templates_.size() != rhs_seq.feature_templates_.size())
            return false;
        for(int i = 0; i < (int)feature_templates_.size(); i++)
            if(feature_templates_[i] != rhs_seq.feature_templates_[i])
                return false;
        return true;
    }

    // Check to make sure that the feature template can be interpreted
    virtual bool FeatureTemplateIsLegal(const std::string & name);

private:

    std::string GetSpanFeatureString(const FeatureDataParse & sent,
                                     int l, int r, const std::string & type);
    double GetSpanFeatureValue(const FeatureDataParse & sent,
                               int l, int r, const std::string & type);

    std::string GetEdgeFeatureString(const FeatureDataParse & sent,
                                     const HyperEdge & edge,
                                     const std::string & type);
    double GetEdgeFeatureValue(const FeatureDataParse & sent,
                               const HyperEdge & edge,
                               const std::string & type);
};

}

#endif

