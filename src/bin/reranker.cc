/*
 * reranker.cc
 *
 *  Created on: Feb 27, 2014
 *      Author: leona
 */




#include <reranker/config-runner.h>
#include <reranker/reranker-runner.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigRunner conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    RerankerRunner runner;
    runner.Run(conf);
}
