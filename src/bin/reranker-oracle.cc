
#include <reranker/config-oracle.h>
#include <reranker/reranker-oracle.h>

using namespace lader;
using namespace std;
using namespace reranker;

int main(int argc, char** argv) {
    // load the arguments
    ConfigOracle conf;
    vector<string> args = conf.loadConfig(argc,argv);
    RerankerOracle evaluator;
    evaluator.Evaluate(conf);
}
