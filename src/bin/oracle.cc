
#include <lader/config-oracle.h>
#include <lader/reorderer-oracle.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigOracle conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    ReordererOracle oracle;
    oracle.Oracle(conf);
}
