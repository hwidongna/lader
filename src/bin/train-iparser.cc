/*
 * train-iparser.cc
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */

#include <shift-reduce-dp/config-iparser-trainer.h>
#include <shift-reduce-dp/iparser-trainer.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigIParserTrainer conf;
    vector<string> args = conf.loadConfig(argc,argv);
    IParserTrainer trainer;
    trainer.TrainIncremental(conf);
}
