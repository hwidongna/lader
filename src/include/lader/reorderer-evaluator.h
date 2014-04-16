#ifndef REORDERER_EVALUATOR_H__ 
#define REORDERER_EVALUATOR_H__

#include <iostream>
#include <fstream>
#include <lader/config-evaluator.h>
#include <lader/combined-alignment.h>

namespace lader {

// A class to train the reordering model
class ReordererEvaluator {
public:

    ReordererEvaluator() : attach_(CombinedAlign::ATTACH_NULL_LEFT),
        combine_(CombinedAlign::COMBINE_BLOCKS),
        bracket_(CombinedAlign::ALIGN_BRACKET_SPANS)
         { }
    virtual ~ReordererEvaluator() { }

    virtual void InitializeModel(const ConfigBase & config);
    // Run the evaluator
    virtual void Evaluate(const ConfigBase & config);

    CombinedAlign::NullHandler attach_; // Where to attach nulls
    CombinedAlign::BlockHandler combine_; // Whether to combine blocks
    CombinedAlign::BracketHandler bracket_; // Whether to handle brackets

};

}

#endif

