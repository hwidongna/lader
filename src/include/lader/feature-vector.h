#ifndef FEATURE_VECTOR_H__ 
#define FEATURE_VECTOR_H__

#include <vector>
#include <string>
#include <climits>
#include <lader/util.h>

namespace lader {

// Real-valued feature vectors that are either indexed with std::strings or ints
typedef std::pair<std::string, double> FeaturePairString;
typedef std::vector<FeaturePairString> FeatureVectorString;
typedef std::pair<int, double> FeaturePairInt;
typedef std::vector<FeaturePairInt> FeatureVectorInt;
typedef std::tr1::unordered_map<int,double> FeatureMapInt;

// Vector subtract
FeatureVectorInt VectorSubtract(const FeatureVectorInt & a, 
                                const FeatureVectorInt & b);


void ClearAndSet(FeatureVectorInt & fvi, const FeatureMapInt & feat_map);

}

#endif

