#ifndef ALIGNMENT_H__ 
#define ALIGNMENT_H__

#include <vector>
#include <iostream>
#include <lader/util.h>
#include <boost/foreach.hpp>

using namespace std;

namespace lader {

// A string of alignments for a particular sentence
class Alignment {
public:
    // Pair of words in an alignment
    typedef pair<int,int> AlignmentPair;

    // Constructor with lengths of the source and target sentence
    Alignment(const AlignmentPair & lens)
        : len_(lens) {
//    	f2e_.resize(len_.first, AlignmentPair(len_.second, -1));
//    	e2f_.resize(len_.second, AlignmentPair(len_.first, -1));
    }

    // Add a single alignment
    void AddAlignment(const AlignmentPair & al) {
#ifdef LADER_SAFE
        if(al.first >= len_.first || al.second >= len_.second)
            THROW_ERROR("Out of bounds in AddAlignment: "<< al << ", " << len_);
#endif
        vec_.push_back(al);
//        // update <min,max> pair
//        AlignmentPair & j = f2e_[al.first];
//        j.first = min(j.first, al.second);
//        j.second = max(j.second, al.second);
//        AlignmentPair & i = e2f_[al.second];
//        i.first = min(i.first, al.first);
//        i.second = max(i.second, al.first);
    }

    // Convert to and from strings
    string ToString() const;
    static Alignment FromString(const string & str);

    // Comparators
    bool operator== (const Alignment & rhs) {
        if(len_ != rhs.len_ ||
           vec_.size() != rhs.vec_.size())
            return false;
        for(int i = 0; i < (int)vec_.size(); i++)
            if(vec_[i] != rhs.vec_[i])
                return false;
        return true;
    }
    bool operator!= (const Alignment & rhs) {
        return !(*this == rhs);
    }

    // ------------- Accessors --------------
    const vector<AlignmentPair> & GetAlignmentVector() const {
        return vec_;
    }
    int GetSrcLen() const { return len_.first; }
    int GetTrgLen() const { return len_.second; }

    void UpdateF(const vector<int> order){
//    	f2e_.clear();
//    	e2f_.clear();
    	BOOST_FOREACH(AlignmentPair & al, vec_){
    		vector<int>::const_iterator it = find(order.begin(), order.end(), al.first);
    		al.first = distance(order.begin(), it);
    	}
    }
private:

    // Split a string in the form X-Y into an alignment pair
    static AlignmentPair SplitAlignment(const string & str);

    // srclen,trglen
    AlignmentPair len_;
    // use the conventional notation
    // f: source, e: target, j: source index, i: target index
    // a list of <j,i> pairs
    vector<AlignmentPair > vec_;
//    // <min,max> pair for each j
//    vector<AlignmentPair > f2e_;
//    // <min,max> pair for each i
//    vector<AlignmentPair > e2f_;
};

}

#endif
