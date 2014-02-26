/*
 * gold-tree.cc
 *
 *  Created on: Feb 26, 2014
 *      Author: leona
 */





#include <reranker/config-gold.h>
#include <reranker/gold-tree.h>

using namespace reranker;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigGold conf;
    vector<string> args = conf.loadConfig(argc,argv);
    GoldTree tree;
    tree.Run(conf);
}
