#ifndef FEATURE_DATA_BASE_H__ 
#define FEATURE_DATA_BASE_H__

#include <vector>
#include <string>
#include <ctype.h>
#include <lader/util.h>
#include <boost/foreach.hpp>

namespace lader {

// A virtual class for the data from a single sentence that is used to calculate
// features. This stores the number of words in a standard format to make it
// easier to check to make sure that all sizes are equal
class FeatureDataBase {
public:
    FeatureDataBase() { }
    virtual ~FeatureDataBase() { }
 
    // Reorder the data according to the input vector
    virtual void Reorder(const std::vector<int> & order) = 0;

    // Convert this data into a string for output
    virtual std::string ToString() const = 0;

    // Accessors
    int GetNumWords() const { return sequence_.size(); }
    const std::vector<std::string> & GetSequence() {
        return sequence_;
    }
    const std::string & GetElement(int i) const {
        return SafeAccess(sequence_, i); 
    }

    const bool isPunct(int i) {
    	BOOST_FOREACH(char c, SafeAccess(sequence_, i).c_str())
    		if (ispunct(c))
    			return true;
    	return false;
    }
protected:
    // The words in the sentence
    std::vector<std::string> sequence_;

private:

};

typedef std::vector<FeatureDataBase*> Sentence;

}

#endif

