/*
 * flat-tree.h
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */

#ifndef FLAT_TREE_H_
#define FLAT_TREE_H_
#include <vector>
#include <shift-reduce-dp/dpstate.h>
#include <lader/hypothesis.h>
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;

namespace reranker {

class Node;
typedef vector<Node*> NodeList;

class Node{
public:
	Node(int left, int right, Node * parent) :
		left_(left), right_(right), parent_(parent) { }
	virtual ~Node(){
		BOOST_FOREACH(Node * node, children_)
			if (node)
				delete node;
	}
	void AddChild(Node * child) { children_.push_back(child); }
	NodeList & GetChildren() { return children_; }
	int GetLeft()  const { return left_; }
	int GetRight() const { return right_; }
	virtual char GetLabel() const = 0;
	bool IsTerminal() const { return children_.empty(); }
	void PrintParse(const vector<string> & strs, ostream & out) const;


protected:
	int left_, right_;	// left (inclusive) and right (exclusive) boundaries
	Node * parent_;		// parent node
	NodeList children_;	// children nodes (may be empty)
};

class DPStateNode : public Node{
	typedef lader::DPState DPState;
public:
	DPStateNode(int left, int right, Node * parent, DPState::Action action) :
		Node(left, right, parent), action_(action) { }
	DPStateNode * Flatten(lader::DPState * root);
	virtual char GetLabel() const { return (char)action_; }
	DPState::Action GetAction() const { return action_; }
private:
	DPState::Action action_;
};

class HypNode : public Node{
	typedef lader::HyperEdge HyperEdge;
public:
	HypNode(int left, int right, Node * parent, HyperEdge::Type type ) :
			Node(left, right, parent), type_(type) { }
	HypNode * Flatten(lader::Hypothesis * root);
	virtual char GetLabel() const { return (char)type_; }
	HyperEdge::Type GetType() const { return type_; }

private:
	HyperEdge::Type type_;
};
}

namespace std {
inline ostream& operator << ( ostream& out,
                                   const reranker::Node & rhs )
{
    out << rhs.GetLabel() << ":"
    	<< "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ">";
    return out;
}

}
#endif /* FLAT_TREE_H_ */
