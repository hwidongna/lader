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
#include <sstream>
#include <iostream>

using namespace lader;

// use Fermat primes
#define HYPEREDGE_NMULT 257
#define HYPEREDGE_MMULT 65537

namespace lader {

class DiscontinuousHyperEdge : public HyperEdge{
public:
	DiscontinuousHyperEdge(int l, int m, int c, int n, int r, Type t):
		HyperEdge(l, c, r, t),
		m_(m), n_(n) { }

	virtual HyperEdge * Clone() const{
		return new DiscontinuousHyperEdge(GetLeft(), m_, GetCenter(), n_, GetRight(), GetType());
	}
	// Comparators
	bool operator< (const HyperEdge & rhs) const {
//		std::cerr << "bool operator< (const HyperEdge & rhs)" << std::endl;
		if (rhs.GetClass() != 'D')
			return HyperEdge::operator< (rhs);
		const DiscontinuousHyperEdge * drhs =
				dynamic_cast<const DiscontinuousHyperEdge*>(&rhs);
		return HyperEdge::operator< (rhs) ||
				(HyperEdge::operator ==(rhs) && (m_ < drhs->m_ || (
				m_ == drhs->m_ && n_ < drhs->n_)));
	}
	bool operator== (const HyperEdge & rhs) const {
		if (rhs.GetClass() != 'D')
			return false;
		const DiscontinuousHyperEdge * drhs =
				dynamic_cast<const DiscontinuousHyperEdge*>(&rhs);
		return HyperEdge::operator ==(rhs) && m_ == drhs->m_ && n_ == drhs->n_;
	}
	// TODO: l <= m < n <= r, can we use this property?
    size_t hash() const {
        return HyperEdge::hash() + m_*HYPEREDGE_MMULT+n_*HYPEREDGE_NMULT;
    }
	int GetM() const { return m_; }
	int GetN() const { return n_; }
	char GetClass() const { return 'D'; }
private:
	int m_;
	int n_;
};

}
namespace std {
// Output function
inline std::ostream& operator << ( std::ostream& out,
                                   const lader::DiscontinuousHyperEdge & rhs )
{
    out << "l=" << rhs.GetLeft() << ", m=" << rhs.GetM()
    		<< ", c=" << rhs.GetCenter() << ", n=" << rhs.GetN()
			<< ", r=" << rhs.GetRight();
    return out;
}
}

#endif /* DISCONTINUOUS_HYPER_EDGE_H_ */
