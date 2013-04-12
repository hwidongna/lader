/*
 * discontinuous-hyper-edge.h
 *
 *  Created on: Apr 10, 2013
 *      Author: leona
 */

#ifndef DISCONTINUOUS_HYPER_EDGE_H_
#define DISCONTINUOUS_HYPER_EDGE_H_

#include <lader/hyper-edge.h>

using namespace lader;

class DiscontinuousHyperEdge : public HyperEdge{
public:
	DiscontinuousHyperEdge(int l, int m, int n, int r, Type t):
		HyperEdge(l, -1, r, t),
		m_(m), n_(n) { }

	// Comparators
	bool operator< (const DiscontinuousHyperEdge & rhs) const {
		return HyperEdge::operator< (rhs) ||
				(HyperEdge::operator ==(rhs) && (
						m_ < rhs.m_ || (m_ == rhs.m_ && n_ < rhs.n_)));
	}
	bool operator== (const DiscontinuousHyperEdge & rhs) const {
		return HyperEdge::operator ==(rhs) && m_ == rhs.m_ && n_ == rhs.n_;
	}

	int GetM() { return m_; }
	int GetN() { return n_; }
private:
	int m_;
	int n_;
};


#endif /* DISCONTINUOUS_HYPER_EDGE_H_ */
