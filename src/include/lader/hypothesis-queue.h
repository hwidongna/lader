#ifndef HYPOTHESIS_QUEUE_H__ 
#define HYPOTHESIS_QUEUE_H__

#include <lader/hypothesis.h>
#include <lader/discontinuous-hypothesis.h>
#include <queue>

namespace lader {

typedef std::priority_queue<Hypothesis> HypothesisQueue;
typedef std::priority_queue<DiscontinuousHypothesis> DiscontinuousHypothesisQueue;

}

#endif

