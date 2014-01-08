/*
 * shift-reduce.cc
 *
 *  Created on: Dec 29, 2013
 *      Author: leona
 */




#include <shift-reduce-dp/config-runner.h>
#include <shift-reduce-dp/shift-reduce-runner.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigRunner conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    ShiftReduceRunner runner;
    runner.Run(conf);
}
