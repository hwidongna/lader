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
	Node() : left_(0), right_(-1), parent_(NULL) { }
	Node(int left, int right, Node * parent) :
		left_(left), right_(right), parent_(parent) { }
	virtual ~Node(){
		BOOST_FOREACH(Node * node, children_)
			if (node)
				delete node;
	}
	void AddChild(Node * child) { child->parent_ = this; children_.push_back(child); }
	Node * GetChild(int i) { return lader::SafeAccess(children_, i); }
	NodeList & GetChildren() { return children_; }
	int GetLeft()  const { return left_; }
	int GetRight() const { return right_; }
	void SetLeft(int left) { left_ = left; }
	void SetRight(int right) { right_ = right; }
	Node * GetParent() const { return parent_; }
	void SetParent(Node * parent) { parent_ = parent; }
	virtual char GetLabel() const = 0;
	int size() const { return right_ - left_; }
	bool IsTerminal() const { return children_.empty(); }
	bool IsRoot() const { return GetLabel() == 'R'; }

	void GetTerminals(NodeList & terminals) const;
	void PrintParse(const vector<string> & strs, ostream & out) const;
	void PrintParse(ostream & out) const;
	inline void MergeChildren(Node * from);

protected:
	int left_, right_;	// left (inclusive) and right (exclusive) boundaries
	Node * parent_;		// parent node
	NodeList children_;	// children nodes (may be empty)
};

class GenericNode : public Node{
public:
	GenericNode(char label) : Node(), label_(label) { }
	GenericNode(int left, int right, Node * parent, char label) :
		Node(left, right, parent), label_(label) { }
	virtual char GetLabel() const { return label_; }
	void SetLabel(char label) { label_ = label; }
protected:
	char label_;
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
