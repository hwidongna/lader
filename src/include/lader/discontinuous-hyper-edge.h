/*
 * discontinuous-hyper-edge.h
 *
 *  Created on: Apr 10, 2013
 *      Author: leona
 */

#ifndef DISCONTINUOUS_HYPER_EDGE_H_
#define DISCONTINUOUS_HYPER_EDGE_H_

#include <lader/hyper-edge.h>
#include <typeinfo> // For std::bad_cast

using namespace lader;

class DiscontinuousHyperEdge : public HyperEdge{
public:
	DiscontinuousHyperEdge(int l, int m, int n, int r, Type t):
		HyperEdge(l, -1, r, t),
		m_(m), n_(n) { }

	// Comparators
	bool operator< (const HyperEdge & rhs) const {
		try{
			const DiscontinuousHyperEdge & drhs =
					dynamic_cast<const DiscontinuousHyperEdge&>(rhs);
			return HyperEdge::operator< (rhs) ||
					(HyperEdge::operator ==(rhs) && (m_ < drhs.m_ || (
					m_ == drhs.m_ && n_ < drhs.n_)));
		}catch (const std::bad_cast& e) {
			return HyperEdge::operator< (rhs);
		}
	}
	bool operator== (const HyperEdge & rhs) const {
		try{
			const DiscontinuousHyperEdge & drhs =
					dynamic_cast<const DiscontinuousHyperEdge&>(rhs);
			return HyperEdge::operator ==(rhs) && m_ == drhs.m_ && n_ == drhs.n_;
		}catch (const std::bad_cast& e){
			return HyperEdge::operator ==(rhs);
		}
	}

	int GetM() const { return m_; }
	int GetN() const { return n_; }
private:
	int m_;
	int n_;
};


#endif /* DISCONTINUOUS_HYPER_EDGE_H_ */
