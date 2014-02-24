/*
 * train-reranker.cc
 *
 *  Created on: Feb 20, 2014
 *      Author: leona
 */




#include <reranker/config-trainer.h>
#include <reranker/reranker-trainer.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigTrainer conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reranker
    RerankerTrainer runner;
    runner.Run(conf);
}
