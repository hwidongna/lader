/*
 * iparser-evaluator.h
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */

#ifndef IPARSER_EVALUATOR_H_
#define IPARSER_EVALUATOR_H_

#include <string>
#include <vector>
#include <shift-reduce-dp/dpstate.h>
#include <lader/config-base.h>
#include <lader/reorderer-evaluator.h>
using namespace std;

namespace lader {


class IParserEvaluator : public ReordererEvaluator{
public:
	IParserEvaluator() : attach_trg_(CombinedAlign::ATTACH_NULL_LEFT) { }
	virtual ~IParserEvaluator() { }

	virtual void InitializeModel(const ConfigBase & config);

	// Run the evaluator
	virtual void Evaluate(const ConfigBase & config);

protected:
	CombinedAlign::NullHandler attach_trg_; // Where to attach nulls

};

} /* namespace lader */


#endif /* IPARSER_EVALUATOR_H_ */
