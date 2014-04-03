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

namespace lader {

class Node;
typedef vector<Node*> NodeList;
typedef vector<const Node*> cNodeList;
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
	Node * GetChild(int i) const { return SafeAccess(children_, i); }
	NodeList GetChildren() const { return children_; }
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
	bool IsStraight() const { return GetLabel() == 'S'; }
	bool IsInverted() const { return GetLabel() == 'I'; }

	void GetTerminals(cNodeList & result) const;
	void GetNonTerminals(cNodeList & result) const;
	void PrintParse(const vector<string> & strs, ostream & out) const;
	void PrintParse(ostream & out) const;
	void PrintString(const vector<string> & strs, ostream & out) const;
	void GetOrder(vector<int> & result);
	inline void MergeChildrenAndDelete(Node * from);
	int NumEdges();

	virtual bool operator==(const Node & rhs) const{
		return left_ == rhs.left_ && right_ == rhs.right_;
	}

	static int Intersection(const Node * t1, const Node * t2);
protected:
	int left_, right_;	// left (inclusive) and right (exclusive) boundaries
	Node * parent_;		// parent node
	NodeList children_;	// children nodes (may be empty)
};




class GenericNode : public Node{
public:
	typedef struct{
		GenericNode* root;  		/* tree's root node */
		vector<string> words;		/* word sequence at terminal nodes */
	} ParseResult;
	GenericNode(char label) : Node(), label_(label) { }
	GenericNode(int left, int right, Node * parent, char label) :
		Node(left, right, parent), label_(label) { }
	virtual char GetLabel() const { return label_; }
	void SetLabel(char label) { label_ = label; }
	static ParseResult * ParseInput(const string & line);
	virtual bool operator==(const Node & rhs) const {
		const GenericNode * gnode = dynamic_cast<const GenericNode*>(&rhs);
		return Node::operator ==(rhs) && gnode && label_ == gnode->label_;
	}
protected:
	char label_;
};

class DPStateNode : public GenericNode{
public:
	DPStateNode(int left, int right, Node * parent, DPState::Action action) :
		GenericNode(left, right, parent, (char)action) { }
	virtual DPStateNode * Flatten(const DPState * root);
};

class DDPStateNode : public DPStateNode{
public:
	DDPStateNode(int left, int right, Node * parent, DPState::Action action) :
		DPStateNode(left, right, parent, action) { }
	virtual DPStateNode * Flatten(const DPState * root);
};

class IDPStateNode : public DPStateNode{
public:
	IDPStateNode(int left, int right, Node * parent, DPState::Action action) :
		DPStateNode(left, right, parent, action) { }
	virtual DPStateNode * Flatten(const DPState * root);
};

class HypNode : public GenericNode{
public:
	HypNode(int left, int right, Node * parent, HyperEdge::Type type ) :
		GenericNode(left, right, parent, (char)type) { }
	HypNode * Flatten(Hypothesis * root);
};
}

namespace std {
inline ostream& operator << ( ostream& out,
                                   const lader::Node & rhs )
{
    out << rhs.GetLabel() << ":"
    	<< "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ">";
    return out;
}

}
#endif /* FLAT_TREE_H_ */
