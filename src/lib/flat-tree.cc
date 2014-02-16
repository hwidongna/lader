/*
 * flat-tree.cc
 *
 *  Created on: Feb 14, 2014
 *      Author: leona
 */
#include <reranker/flat-tree.h>
#include <lader/util.h>

using namespace reranker;
using namespace lader;

DPStateNode * DPStateNode::Flatten(lader::DPState * root){
	if (!root) // just in case
		return NULL;
	DPStateNode * v = new DPStateNode(root->GetSrcL(), root->GetSrcR(), this, root->GetAction());
	if (root->GetAction() != lader::DPState::SHIFT){ // this is non-terminal
		lader::DPState * lchild = root->LeftChild();
		if (!lchild)
			THROW_ERROR("NULL left child ")
			DPStateNode * l = v->Flatten(lchild);
		if ( l->action_ == v->action_ ){
			v->children_.insert(v->children_.end(), l->children_.begin(), l->children_.end());
			l->children_.clear();
			delete l;
		}
		else
			v->AddChild(l);
		lader::DPState * rchild = root->RightChild();
		if (!rchild)
			THROW_ERROR("NULL right child")
		DPStateNode * r = v->Flatten(rchild);
		if (r->action_ == v->action_ ){
			v->children_.insert(v->children_.end(), r->children_.begin(), r->children_.end());
			r->children_.clear();
			delete r;
		}
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
		if ( l->type_ == v->type_ ){
			v->children_.insert(v->children_.end(), l->children_.begin(), l->children_.end());
			l->children_.clear();
			delete l;
		}
		else
			v->AddChild(l);
		lader::Hypothesis * rchild = root->GetRightHyp();
		if (!rchild && root->GetEdgeType())
			THROW_ERROR("NULL right child")
		HypNode * r = v->Flatten(rchild);
		if (r->type_ == v->type_ ){
			v->children_.insert(v->children_.end(), r->children_.begin(), r->children_.end());
			r->children_.clear();
			delete r;
		}
		else
			v->AddChild(r);
	}
	return v;
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
