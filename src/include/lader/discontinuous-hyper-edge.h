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

namespace lader {

class DiscontinuousHyperEdge : public HyperEdge{
public:
	DiscontinuousHyperEdge(int l, int m, int n, int r, Type t):
		HyperEdge(l, -1, r, t),
		m_(m), n_(n) { }

	HyperEdge * Clone() const{
		return new DiscontinuousHyperEdge(GetLeft(), m_, n_, GetRight(), GetType());
	}
	// Comparators
	bool operator< (const HyperEdge & rhs) const {
		std::cerr << "bool operator< (const HyperEdge & rhs)" << std::endl;
		try{
			const DiscontinuousHyperEdge & drhs =
					dynamic_cast<const DiscontinuousHyperEdge&>(rhs);
			return HyperEdge::operator< (rhs) ||
					(HyperEdge::operator ==(rhs) && (m_ < drhs.m_ || (
					m_ == drhs.m_ && n_ < drhs.n_)));
		}catch (const std::bad_cast& e) { // if rhs is base class instance
		}
		return HyperEdge::operator< (rhs);
	}
	bool operator== (HyperEdge & rhs) const {
		try{
			DiscontinuousHyperEdge & drhs =
					dynamic_cast<DiscontinuousHyperEdge&>(rhs);
			return HyperEdge::operator ==(rhs) && m_ == drhs.m_ && n_ == drhs.n_;
		}catch (const std::bad_cast& e){ // if rhs is base class instance
		}
		return HyperEdge::operator ==(rhs);
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
    out << "l=" << rhs.GetLeft() << ", m=" << rhs.GetM() << ", n=" << rhs.GetN() << ", r=" << rhs.GetRight();
    return out;
}
}

#endif /* DISCONTINUOUS_HYPER_EDGE_H_ */
