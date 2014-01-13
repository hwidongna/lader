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
    // A template for the features given the type, and a vector containing
    // the feature name and the values to be replaced
    typedef std::pair<FeatureType, std::vector<std::string> > FeatureTemplate;

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
    //  %[SLR]S :    entire String of a span
    //  %[SLR][LR] : Left or Right-most word of a span
    //  %[SLR]N :    Number of words in a span
    //  %[SLR]QE[0-9]: Whether a span exists in dictionary numbered 0-9,
    //               specified with dict=XX
    //  %[SLR]Q#[0-9][0-9]: The value of the [0-9]th feature in the [0-9]th
    //               dictionary (dictionary number is first, feature second)
    //  %CD :        Difference (absolute value) in words in two spans
    //  %ET :        The type of the edge
    // 
    // In addition N and D can be used with at the beginning (eg %SN#) to
    // fire non-binary features. These cannot be combined with other features
    //
    // For shift-reduce parsing
    // The templates that can be used will all start with:
    //  S: For feetures designating a stack element
    //  L or R: For features defined over the left or right child of a stack element
    //  Q: For feetures designating a buffer element
    //
    // The templates that can be used and combined for binary features are:
    //  %s[0-9]: the [0-9]th element in stack (from top)
    //  %[lr][0-9]: the left/right child or the [0-9]th stack element
    //  %aT :        The action type: SHIFT(0), STRAIGHT(1), INVERTED(2)
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

    // Generates the features that can be factored over a state
	virtual void GenerateStateFeatures(
								const FeatureDataBase & sentence,
								const DPState & state,
								const DPState::Action & action,
								SymbolSet<int> & feature_ids,
								bool add,
								FeatureVectorInt & feats);

    // Get the type of this feature generator
    virtual std::string GetType() const { return "seq"; }

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
    static bool FeatureTemplateIsLegal(const std::string & name);

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

    std::vector<FeatureTemplate> feature_templates_;

};

}

#endif

