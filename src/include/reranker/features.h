/*
 * features.h
 *
 *  Created on: Feb 16, 2014
 *      Author: leona
 */

#ifndef FEATURES_H_
#define FEATURES_H_
#include <shift-reduce-dp/flat-tree.h>
#include <boost/foreach.hpp>
#include <reranker/reranker-model.h>
using namespace lader;

namespace reranker{

template <typename Type>
class FeatureBase{
public:
	virtual void Set(FeatureMapInt & feat, RerankerModel & model) = 0;
	virtual ~FeatureBase() { }
	Type & GetFeature() { return feature_; }
protected:
	Type feature_;
};

class NLogP: public FeatureBase<double>{
public:
	NLogP(double value) : value_(value) { }
	virtual void Set(FeatureMapInt & feat, RerankerModel & model) {
		int id = model.GetId("NLogP", model.GetAdd());
		FeatureMapInt::iterator it = feat.find(id);
		if (it == feat.end())
			feat[id] = value_;
		else
			it->second += value_;

	}
private:
	double value_;
};


class TreeFeature: public FeatureBase< vector<string> >{
public:
	TreeFeature() : parent_lexical_(false), child_lexical_(false) { }
	virtual ~TreeFeature() { }
	// prefix of feature name, see below
	virtual string FeatureName() = 0;
	// possibly multiple features
	virtual void Set(FeatureMapInt & feat, RerankerModel & model){
		ostringstream oss;
		BOOST_FOREACH(string f, feature_){
			oss << FeatureName() << ":" << f << std::ends; // using prefix
			int id = model.GetId(oss.str().data(), model.GetAdd());
			if (id < 0)
				continue;
			FeatureMapInt::iterator it = feat.find(id);
			if (it == feat.end())
				feat[id] = 1;
			else
				it->second++;
			oss.seekp(0);
		}
	}
	void SetLexical(bool parent, bool child) {
		parent_lexical_ = parent;
		child_lexical_ = child;
	}
	virtual void Extract(Node * node, const lader::FeatureDataBase & sentence) = 0;
protected:
	bool parent_lexical_;	// whether include lexical information or not in parent
	bool child_lexical_;	// whether include lexical information or not in child
};

typedef vector<TreeFeature*> TreeFeatureSet;

// These features come from Charniak and Johnson (ACL-2005) and Collins and Koo (CL-2005)
class Rule: public TreeFeature{
	typedef string Feature;
public:
	virtual string FeatureName() { return "Rule";  }
	virtual void Extract(Node * root, const lader::FeatureDataBase & sentence){
		if (root->IsTerminal())
			return;
		ostringstream oss;
		oss << root->GetLabel();
		if (parent_lexical_)
			oss << "(" << sentence.GetElement(root->GetLeft())
				<< "," << sentence.GetElement(root->GetRight()-1) << ")";
		oss << "-";
		int i = 0;
		BOOST_FOREACH(Node * child, root->GetChildren()){
			Extract(child, sentence);
			if (i++ != 0) oss << ".";
			oss << child->GetLabel();
			if (child_lexical_)
				oss << "(" << sentence.GetElement(child->GetLeft())
					<< "," << sentence.GetElement(child->GetRight()-1) << ")";
		}
		feature_.push_back(oss.str());
	}
};

class Word: public TreeFeature{
public:
	Word(int level) : level_(level) { }
	virtual string FeatureName() { return "Word"; }
	virtual void Extract(Node * root, const lader::FeatureDataBase & sentence){
		cNodeList terminals;
		root->GetTerminals(terminals);
		ostringstream oss;
		BOOST_FOREACH(const Node * node, terminals){
			oss << "(";
			for (int i = node->GetLeft() ; i < node->GetRight() ; i++){
				if (i != node->GetLeft()) oss << ",";
				oss << sentence.GetElement(i);
			}
			oss << ")";
			for (int i = 1 ; i < level_ && node->GetParent() ; i++){
				node = node->GetParent();
				oss << "/";
				oss << node->GetLabel();
				if (parent_lexical_)
					oss << "(" << sentence.GetElement(node->GetLeft())
						<< "," << sentence.GetElement(node->GetRight()-1) << ")";
			}
			oss << std::ends;
			feature_.push_back(oss.str().data());
			oss.seekp(0);
		}
	}
private:
	int level_;
};

class NGram: public TreeFeature{
public:
	NGram(int order) : order_(order) { }
	virtual string FeatureName() { return "NGram"; }
	virtual void Extract(Node * node, const lader::FeatureDataBase & sentence){
		NodeList children = node->GetChildren();
		ostringstream oss;
		for (int i = 0 ; i <= children.size() ; i++){
			oss << node->GetLabel();
			if (parent_lexical_)
				oss << "(" << sentence.GetElement(node->GetLeft())
					<< "," << sentence.GetElement(node->GetRight()-1) << ")";
			oss << "-";
			for (int j = i-order_+1; j < i ; j++){
				if ( j == -1 )
					oss << "[" << ".";
				else if (j >= 0){
					Node * child = children[j];
					oss << child->GetLabel() << ".";
					if (child_lexical_)
						oss << "(" << sentence.GetElement(child->GetLeft())
							<< "," << sentence.GetElement(child->GetRight()-1) << ")";
				}
			}
			if ( i == children.size() )
				oss <<  "]";
			else{
				Node * child = children[i];
				oss << child->GetLabel();
				if (child_lexical_)
					oss << "(" << sentence.GetElement(child->GetLeft())
						<< "," << sentence.GetElement(child->GetRight()-1) << ")";
			}
			oss << std::ends;
			feature_.push_back(oss.str().data());
			oss.seekp(0);
		}

	}
protected:
	int order_;
};

class NGramTree: public NGram{
public:
	NGramTree(int order) : NGram(order) { }
	virtual string FeatureName() { return "NGramTree"; }
	virtual void Extract(Node * root, const lader::FeatureDataBase & sentence){
		cNodeList terminals;
		root->GetTerminals(terminals);
		ostringstream oss;
		for (int i = 0 ; i+order_ <= root->size() ; i++){
			const Node * lca = terminals[i];
			int l = lca->GetLeft();
			while(lca && !lca->IsRoot() && lca->GetRight() < l+order_){
				lca = lca->GetParent();
			}
			Node * node = SelectiveCopy(lca, NULL, l, l+order_);
			node->PrintParse(sentence.GetSequence(), oss);
			oss << std::ends;
			feature_.push_back(oss.str().data());
			delete node;
			oss.seekp(0);
		}
	}
	Node * SelectiveCopy(const Node * lca, Node * parent, int left, int right){
		Node * node = new GenericNode(left, right, parent, lca->GetLabel());
		BOOST_FOREACH(Node * child, lca->GetChildren()){
			if (child->GetRight() <= left ) // left exclusive
				continue;
			else if (child->GetLeft() < left && child->GetRight() <= right) // left overlap
				node->AddChild(SelectiveCopy(child, node, left, child->GetRight()));
			else if (child->GetLeft() < right && child->GetRight() > right) // right overlap
				node->AddChild(SelectiveCopy(child, node, child->GetLeft(), right));
			else if (right <= child->GetLeft()) // right exclusive
				break;
			else // inclusive
				node->AddChild(SelectiveCopy(child, node, child->GetLeft(), child->GetRight()));
		}
		return node;
	}
};

}

#endif /* FEATURES_H_ */
