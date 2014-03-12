/*
 * gold-tree.cc
 *
 *  Created on: Feb 26, 2014
 *      Author: leona
 */





#include <shift-reduce-dp/config-gold.h>
#include <shift-reduce-dp/gold-standard.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigGold conf;
    vector<string> args = conf.loadConfig(argc,argv);
    GoldStandard gold;
    gold.Run(conf);
}
