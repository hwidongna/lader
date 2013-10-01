#ifndef TEST_BASE__
#define TEST_BASE__

#include <vector>
#include <iostream>
#include <cmath>
#include <lader/feature-vector.h>
#include <target-span.h>
using namespace std;

namespace lader {

class TestBase {

public:

    TestBase() : passed_(false) { }
    virtual ~TestBase() { }

    // RunTest must be implemented by any test, and returns true if all
    // tests were passed
    virtual bool RunTest() = 0;

protected:

    bool passed_;

    template<class T>
    int CheckVector(const std::vector<T> & exp, const std::vector<T> & act) {
        int ok = 1;
        for(int i = 0; i < (int)max(exp.size(), act.size()); i++) {
            if(i >= (int)exp.size() || 
               i >= (int)act.size() || 
               exp[i] != act[i]) {
               
                ok = 0;
                std::cout << "exp["<<i<<"] != act["<<i<<"] (";
                if(i >= (int)exp.size()) std::cout << "NULL";
                else std::cout << exp[i];
                std::cout <<" != ";
                if(i >= (int)act.size()) std::cout << "NULL"; 
                else std::cout << act[i];
                std::cout << ")" << std::endl;
            }
        }
        return ok;
    }

    template<class T>
    int CheckAlmostVector(const std::vector<T> & exp,
                          const std::vector<T> & act) {
        int ok = 1;
        for(int i = 0; i < (int)max(exp.size(), act.size()); i++) {
            if(i >= (int)exp.size() || 
               i >= (int)act.size() || 
               abs(exp[i] - act[i]) > 0.01) {
               
                ok = 0;
                std::cout << "exp["<<i<<"] != act["<<i<<"] (";
                if(i >= (int)exp.size()) std::cout << "NULL";
                else std::cout << exp[i];
                std::cout <<" != ";
                if(i >= (int)act.size()) std::cout << "NULL"; 
                else std::cout << act[i];
                std::cout << ")" << std::endl;
            }
        }
        return ok;
    }

    int CheckString(const std::string & exp, const std::string & act) {
        if(exp != act) {
            cerr << "CheckString failed" << endl << "exp: '"<<exp<<"'"
                 <<endl<<"act: '"<<act<<"'" <<endl;
            for(int i = 0; i < (int)min(exp.length(), act.length()); i++)
                if(exp[i] != act[i])
                    cerr << "exp[" << i << "] '" << exp[i] << "' != act["<<i<<"] '"<<act[i]<<"'" <<endl;
            return 0;
        }
        return 1;
    }


    int CheckStackEqual(TargetSpan * stack1, TargetSpan * stack2){
    	int l = stack1->GetLeft();
    	int r = stack1->GetRight();
    	int n = l == r ? 1 : r-l+2;
    	FeatureVectorInt * fvi1, *fvi2;
    	int ret = 1;
    	for (int c = 0 ; c < n ; c++){
    		fvi1 = stack1->GetStraightFeautures(c);
    		fvi2 = stack2->GetStraightFeautures(c);
    		if (fvi1 && fvi2)
    			ret = CheckVector(*fvi1, *fvi2);
    		else if (fvi1 && !fvi1->empty()){
    			int i = 0;
    			cerr << "Straight features are not restored " << *stack1 << endl;
    			BOOST_FOREACH(FeaturePairInt & feat, *fvi1){
    				if (i++ != 0) cerr << " ";
    				cerr << feat.first << ":" << feat.second;
    			}
    			cerr << endl;
    			ret = 0;
    		}
    		fvi1 = stack1->GetInvertedFeautures(c);
    		fvi2 = stack2->GetInvertedFeautures(c);
    		if (fvi1 && fvi2)
    			ret = CheckVector(*fvi1, *fvi2);
    		else if (fvi1 && !fvi1->empty()){
    			int i = 0;
    			cerr << "Inverted features are not restored " << *stack1 << endl;
    			BOOST_FOREACH(FeaturePairInt & feat, *fvi1){
    				if (i++ != 0) cerr << " ";
    				cerr << feat.first << ":" << feat.second;
    			}
    			cerr << endl;
    			ret = 0;
    		}
    	}
    	return ret;
    }
};

}

#endif
