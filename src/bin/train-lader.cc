
#include <lader/config-trainer.h>
#include <lader/reorderer-trainer.h>
#include <shift-reduce-dp/shift-reduce-trainer.h>

using namespace lader;
using namespace std;

int main(int argc, char** argv) {
    // load the arguments
    ConfigTrainer conf;
    vector<string> args = conf.loadConfig(argc,argv);
    // train the reorderer
    if (conf.GetString("algorithm") == "linear"){ // shift-reduce
    	ShiftReduceTrainer trainer;
    	trainer.TrainIncremental(conf);
    }
    else{ // CYK
    	ReordererTrainer trainer;
    	trainer.TrainIncremental(conf);
    }
}
