/*
 * iparser-gold.cc
 *
 *  Created on: Apr 1, 2014
 *      Author: leona
 */




#include <shift-reduce-dp/config-iparser-gold.h>
#include <shift-reduce-dp/iparser-gold.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigIParserGold conf;
    vector<string> args = conf.loadConfig(argc,argv);
    IParserGold gold;
    gold.Run(conf);
}
