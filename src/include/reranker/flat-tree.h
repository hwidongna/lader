/*
 * flat-tree.h
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */

#ifndef FLAT_TREE_H_
#define FLAT_TREE_H_
#include <list>
#include <shift-reduce-dp/dpstate.h>
#include <lader/hypothesis.h>
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;

namespace reranker {

class Node;
typedef list<Node*> NodeList;

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
	virtual void PrintParse(const vector<string> & strs, ostream & out) const = 0;


protected:
	int left_, right_;
	Node * parent_;
	NodeList children_;
};

class DPStateNode : public Node{
public:
	DPStateNode(int left, int right, Node * parent, lader::DPState::Action action) :
		Node(left, right, parent), action_(action) { }
	DPStateNode * Flatten(lader::DPState * root);
	void PrintParse(const vector<string> & strs, ostream & out) const;
	lader::DPState::Action GetAction() const { return action_; }
private:
	lader::DPState::Action action_;
};

class HypNode : public Node{
public:
	HypNode(int left, int right, Node * parent, lader::HyperEdge::Type type ) :
			Node(left, right, parent), type_(type) { }
	HypNode * Flatten(lader::Hypothesis * root);
	void PrintParse(const vector<string> & strs, ostream & out) const;
	lader::HyperEdge::Type GetType() const { return type_; }
private:
	lader::HyperEdge::Type type_;
};
}

namespace std {
inline ostream& operator << ( ostream& out,
                                   const reranker::DPStateNode & rhs )
{
    out << rhs.GetAction() << ":"
    	<< "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ">";
    return out;
}

inline ostream& operator << ( ostream& out,
                                   const reranker::HypNode & rhs )
{
    out << rhs.GetType() << ":"
    	<< "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ">";
    return out;
}
}
#endif /* FLAT_TREE_H_ */
