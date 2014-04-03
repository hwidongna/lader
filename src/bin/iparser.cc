/*
 * iparser.cc
 *
 *  Created on: Apr 3, 2014
 *      Author: leona
 */




#include <shift-reduce-dp/config-iparser-runner.h>
#include <shift-reduce-dp/iparser-runner.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigIParserRunner conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    IParserRunner runner;
    runner.Run(conf);
}
