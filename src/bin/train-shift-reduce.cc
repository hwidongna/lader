
#include <shift-reduce-dp/config-trainer.h>
#include <shift-reduce-dp/shift-reduce-trainer.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigTrainer conf;
    vector<string> args = conf.loadConfig(argc,argv);
    ShiftReduceTrainer trainer;
    trainer.TrainIncremental(conf);
}
