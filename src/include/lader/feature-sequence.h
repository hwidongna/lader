#ifndef FEATURE_SEQUENCE_H__ 
#define FEATURE_SEQUENCE_H__

#include <vector>
#include <string>
#include <lader/feature-data-sequence.h>
#include <lader/feature-base.h>
#include <lader/dictionary.h>

namespace lader {

// A class to calculate features that concern sequences of words, tags, etc.
class FeatureSequence : public FeatureBase {
public:
    typedef std::pair<FeatureType,std::vector<std::string> > FeatureTemplate;
    FeatureSequence()
    {
    }

    virtual ~FeatureSequence()
    {
        BOOST_FOREACH(Dictionary* dict, dicts_)
    		delete dict;
    }

    virtual void ParseConfiguration(const std::string & str);
    virtual FeatureDataBase *ParseData(const std::string & str);
    virtual void GenerateEdgeFeatures(const FeatureDataBase & sentence, const HyperEdge & edge, SymbolSet<int> & feature_ids, bool add, FeatureVectorInt & feats);
    virtual std::string GetType() const
    {
        return "seq";
    }

    virtual bool CheckEqual(const FeatureBase & rhs) const
    {
        if(rhs.GetType() != this->GetType())
            return false;

        const FeatureSequence & rhs_seq = (const FeatureSequence& )(rhs);
        if(feature_templates_.size() != rhs_seq.feature_templates_.size())
            return false;

        for(int i = 0;i < (int)(feature_templates_.size());i++)
            if(feature_templates_[i] != rhs_seq.feature_templates_[i])
                return false;


        return true;
    }
    static bool FeatureTemplateIsLegal(const std::string & name);
    friend class TestDiscontinuousHyperGraph;
private:
    std::string GetSpanFeatureString(const FeatureDataSequence & sent, int l, int r, const std::string & type);
    double GetSpanFeatureValue(const FeatureDataSequence & sent, int l, int r, const std::string & type);
    std::string GetEdgeFeatureString(const FeatureDataSequence & sent, const HyperEdge & edge, const std::string & type);
    double GetEdgeFeatureValue(const FeatureDataSequence & sent, const HyperEdge & edge, const std::string & type);

    std::vector<FeatureTemplate> feature_templates_;
    std::vector<Dictionary*> dicts_;

};

}

#endif

