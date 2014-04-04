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
using namespace std;

namespace lader {


class IParserEvaluator{
public:
	IParserEvaluator() { }
	virtual ~IParserEvaluator() { }

	// Read in the alignments
	void ReadGold(const string & gold_in, vector<ActionVector> & golds);

	// Obtain a merged action sequence from bidirectional gold action sequences
    void GetMergedReference(ActionVector & refseq,
			const ActionVector & frefseq, const ActionVector & erefseq,
			int verbose);
	// Run the evaluator
	void Evaluate(const ConfigBase & config);

protected:
	vector<ActionVector> source_gold_;
	vector<ActionVector> target_gold_;

};

} /* namespace lader */


#endif /* IPARSER_EVALUATOR_H_ */
