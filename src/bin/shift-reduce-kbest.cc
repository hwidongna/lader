/*
 * shift-reduce.cc
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */




#include <shift-reduce-dp/config-runner.h>
#include <shift-reduce-dp/shift-reduce-kbest.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigRunner conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    ShiftReduceKbest runner;
    runner.Run(conf);
}
