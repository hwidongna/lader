/*
 * evaluate-iparser.cc
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */


#include <shift-reduce-dp/config-iparser-evaluator.h>
#include <shift-reduce-dp/iparser-evaluator.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigIParserEvaluator conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    IParserEvaluator evaluator;
    evaluator.Evaluate(conf);
}


