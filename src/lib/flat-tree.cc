/*
 * flat-tree.cc
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */
#include <reranker/flat-tree.h>
#include <lader/util.h>
#include <stack>

using namespace reranker;
using namespace lader;

inline void Node::MergeChildren(Node * from){
	BOOST_FOREACH(Node * child, from->children_){
		AddChild(child);
	}
	from->children_.clear();
	delete from;
}

int Node::NumEdges(){
	int count = children_.empty() ? 0 : 1;
	BOOST_FOREACH(Node * child, children_){
		count += child->NumEdges();
	}
	return count;

}
DPStateNode * DPStateNode::Flatten(lader::DPState * root){
	if (!root) // just in case
		return NULL;
	DPStateNode * v = new DPStateNode(root->GetSrcL(), root->GetSrcR(), this, root->GetAction());
	if (root->GetAction() != lader::DPState::SHIFT){ // this is non-terminal
		lader::DPState * lchild = root->LeftChild();
		if (!lchild)
			THROW_ERROR("NULL left child ")
		DPStateNode * l = v->Flatten(lchild);
		if ( l->label_ == v->label_ )
			v->MergeChildren(l);
		else
			v->AddChild(l);
		lader::DPState * rchild = root->RightChild();
		if (!rchild)
			THROW_ERROR("NULL right child")
		DPStateNode * r = v->Flatten(rchild);
		if (r->label_ == v->label_ )
			v->MergeChildren(r);
		else
			v->AddChild(r);
	}
	return v;
}

HypNode * HypNode::Flatten(lader::Hypothesis * root){
	if (!root) // just in case
		return NULL;
	HypNode * v = new HypNode(root->GetLeft(), root->GetRight()+1, this, root->GetEdgeType());
	if (!root->IsTerminal()){ // this is non-terminal
		lader::Hypothesis * lchild = root->GetLeftHyp();
		if (!lchild)
			THROW_ERROR("NULL left child ")
		HypNode * l = v->Flatten(lchild);
		if ( l->label_ == v->label_ )
			v->MergeChildren(l);
		else
			v->AddChild(l);
		lader::Hypothesis * rchild = root->GetRightHyp();
		if (!rchild && root->GetEdgeType())
			THROW_ERROR("NULL right child")
		HypNode * r = v->Flatten(rchild);
		if (r->label_ == v->label_ )
			v->MergeChildren(r);
		else
			v->AddChild(r);
	}
	return v;
}

void Node::GetTerminals(NodeList & result) const{
	BOOST_FOREACH(Node * child, children_){
		if (child->IsTerminal())
			result.push_back(child);
		else
			child->GetTerminals(result);
	}
}

void Node::GetNonTerminals(NodeList & result) {
	result.push_back(this);
	BOOST_FOREACH(Node * child, children_){
		if (!child->IsTerminal()){
			child->GetNonTerminals(result);
		}
	}
}

int Node::Intersection(Node * t1, Node * t2){
	int count = 0;
	NodeList e1, e2;
	t1->GetNonTerminals(e1); t2->GetNonTerminals(e2);
	NodeList::iterator v1 = e1.begin(), v2 = e2.begin();
	while (v1 != e1.end() && v2 != e2.end()){
		const Node & n1 = SafeReference(*v1);
		const Node & n2 = SafeReference(*v2);
		if (n1 == n2){
			count++;
			v1++; v2++;
		}
		else if (n1.GetLeft() < n2.GetLeft())
			v1++;
		else
			v2++;
	}
	return count;
}

void Node::PrintParse(const vector<string> & strs, ostream & out) const{
    if(IsTerminal()) {
        out << "(" << GetLabel();
        for(int i = GetLeft(); i < GetRight(); i++)
            out << " " << GetTokenWord(strs[i]);
        out << ")";
    } else {
    	out << "(" << GetLabel();
		BOOST_FOREACH(Node * child, children_){
			if (!child)
				THROW_ERROR("Invalid children for node " << *this << endl)
			out << " ";
			child->PrintParse(strs, out);
		}
		out << ")";
    }
}

void Node::PrintParse(ostream & out) const{
    if(IsTerminal()) {
        out << "(" << GetLabel();
        for(int i = GetLeft(); i < GetRight(); i++)
            out << " " << i;
        out << ")";
    } else {
    	out << "(" << GetLabel();
		BOOST_FOREACH(Node * child, children_){
			if (!child)
				THROW_ERROR("Invalid children for node " << *this << endl)
			out << " ";
			child->PrintParse(out);
		}
		out << ")";
    }
}


GenericNode::ParseResult * GenericNode::ParseInput(const string & line){
	stack<GenericNode*> s;
	const char * str = line.c_str();
	int i = 0;
	ParseResult * result = NULL;
	int n = line.size();
	int begin;
	for (begin = 0 ; begin < n ;){
		if (str[begin] == '(' && begin+1 < n
		&& (str[begin+1] == 'R' || str[begin+1] == 'S' || str[begin+1] == 'I' || str[begin+1] == 'F')){
			GenericNode * root = new GenericNode(str[begin+1]);
			if (!result){
				result = new ParseResult;
				result->root = root;
			}
			s.push(root);
			begin += 2;
		}
		else if (str[begin] == ')'){
			begin++;
			GenericNode * child = s.top();
			if (!child->IsTerminal()){
				child->SetLeft(child->GetChildren().front()->GetLeft());
				child->SetRight(child->GetChildren().back()->GetRight());
			}
			s.pop();
			if (s.empty())
				break;
			s.top()->AddChild(child);
		}
		else if (str[begin] != ' '){
			s.top()->SetLeft(i++);
			s.top()->SetRight(i);
			int length = line.find(")", begin)-begin;
			result->words.push_back(line.substr(begin, length));
			begin += length;
		}
		else if (str[begin] == ' ')
			begin++;
		else
			THROW_ERROR("Unexpected input: " << line << endl);
	}
	if (begin < n || !s.empty())
		THROW_ERROR("Unexpected input: " << line << endl);
	return result;
}
