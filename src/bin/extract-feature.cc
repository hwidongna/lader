/*
 * train-reranker.cc
 *
 *  Created on: Feb 20, 2014
 *      Author: leona
 */




#include <reranker/config-extractor.h>
#include <reranker/feature-extractor.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigExtractor conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // extract features for reranker
    FeatureExtractor runner;
    runner.Run(conf);
}
