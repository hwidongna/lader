/*
 * feature-bilingual.h
 *
 *  Created on: Oct 14, 2013
 *      Author: leona
 */

#ifndef FEATURE_BILINGUAL_H_
#define FEATURE_BILINGUAL_H_

#include <lader/feature-sequence.h>
#include <lader/combined-alignment.h>

namespace lader {

class FeatureBilingual : public FeatureSequence{
public:
	FeatureBilingual() {}
	virtual ~FeatureBilingual() {}

	virtual void GenerateBilingualFeatures(const FeatureDataBase & source,
			const FeatureDataBase & target, const CombinedAlign & align,
			const HyperEdge & edge, SymbolSet<int> & feature_ids, bool add,
			FeatureVectorInt & feats);
    virtual bool FeatureTemplateIsLegal(const std::string & name);
protected:
    double GetFeatureValue(const FeatureDataSequence & f,
			const FeatureDataSequence & e, const CombinedAlign & align,
			const HyperEdge & edge, const std::string & str);
    std::string GetFeatureString(const FeatureDataSequence & f,
			const FeatureDataSequence & e, const CombinedAlign & align,
			const HyperEdge & edge, const std::string & type);
private:
    friend class TestFeatureBilingual;
    int GetMinF2E(char type, const HyperEdge & edge, const CombinedAlign & align);
    int GetMaxF2E(char type, const HyperEdge & edge, const CombinedAlign & align);
};

} /* namespace lader */
#endif /* FEATURE_BILINGUAL_H_ */
