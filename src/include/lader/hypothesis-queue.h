#ifndef HYPOTHESIS_QUEUE_H__ 
#define HYPOTHESIS_QUEUE_H__

#include <lader/hypothesis.h>
#include <lader/discontinuous-hypothesis.h>
#include <queue>

namespace lader {

template< typename T >
class pointer_comparator : public std::binary_function< T, T, bool >{
public :
bool operator()( T x, T y ) const { return *x < *y; }
};

typedef std::priority_queue<Hypothesis*,
		std::vector<Hypothesis*>, pointer_comparator<Hypothesis*> > HypothesisQueue;

}

#endif

