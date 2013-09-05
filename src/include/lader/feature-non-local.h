#ifndef FEATURE_NON_LOCAL_H__
#define FEATURE_NON_LOCAL_H__

#include <vector>
#include <string>
#include <lader/feature-data-sequence.h>
#include <lader/feature-base.h>

namespace lader {

// A class to calculate features that concern non-local
class FeatureNonLocal : public FeatureBase {
public:
    FeatureNonLocal() {}

    virtual ~FeatureNonLocal() {
    }

    virtual void ParseConfiguration(const std::string & str);
    virtual FeatureDataBase *ParseData(const std::string & str);
    // Generates the features that can be factored over a hypothesis
    virtual void GenerateNonLocalFeatures(const FeatureDataBase & sentence, const Hypothesis & left, const Hypothesis & right, SymbolSet<int> & feature_ids, bool add, FeatureVectorInt & feats);
    virtual std::string GetType() const
    {
        return "non-local";
    }

    virtual bool CheckEqual(const FeatureBase & rhs) const
    {
        if(rhs.GetType() != this->GetType())
            return false;

        const FeatureNonLocal & rhs_seq = (const FeatureNonLocal& )((rhs));
        if(feature_templates_.size() != rhs_seq.feature_templates_.size())
            return false;

        for(int i = 0;i < (int)((feature_templates_.size()));i++)
            if(feature_templates_[i] != rhs_seq.feature_templates_[i])
                return false;


        return true;
    }
    virtual bool FeatureTemplateIsLegal(const std::string & name);
private:
    std::string GetNonLocalFeatureString(const FeatureDataSequence & sent, int l, int r, const std::string & type);
};

}

#endif

