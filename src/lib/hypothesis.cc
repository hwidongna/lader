/*
 * hypothesis.cc
 *
 *  Created on: May 2, 2013
 *      Author: leona
 */

#include <hypothesis.h>
#include <target-span.h>
#include <discontinuous-target-span.h>
#include <sstream>

using namespace std;
using namespace lader;

inline string GetTokenWord(const string & str) {
    ostringstream oss;
    for(int i = 0; i < (int)str.length(); i++) {
        switch (str[i]) {
            case '(': oss << "-LRB-"; break;
            case ')': oss << "-RRB-"; break;
            case '[': oss << "-LSB-"; break;
            case ']': oss << "-RSB-"; break;
            case '{': oss << "-LCB-"; break;
            case '}': oss << "-RCB-"; break;
            default: oss << str[i];
        }
    }
    return oss.str();
}

void Hypothesis::PrintChildren( std::ostream& out ) const{
	if (left_child_ && left_rank_ < left_child_->GetHypotheses().size()){
		out << endl << "\t" << left_rank_ << "th LChild ";
		DiscontinuousHypothesis * left =
				dynamic_cast<DiscontinuousHypothesis*>(left_child_->GetHypothesis(left_rank_));
		if (left)
			out << *left;
		else
			out << *left_child_->GetHypothesis(left_rank_);
	}
	if (right_child_ && right_rank_ < right_child_->GetHypotheses().size()){
		out << endl << "\t" << right_rank_ << "th RChild ";
		DiscontinuousHypothesis * right =
				dynamic_cast<DiscontinuousHypothesis*>(right_child_->GetHypothesis(right_rank_));
		if (right)
			out << *right;
		else
			out << *right_child_->GetHypothesis(right_rank_);
	}
	out << endl;
}

void Hypothesis::PrintParse(ostream & out) const {
    HyperEdge::Type type = GetEdgeType();
    if(IsTerminal()) {
        out << "(" << (char)type;
        for(int i = GetLeft(); i <= GetRight(); i++)
            out << " ("<<(char)type<< "W " << i <<")";
        out << ")";
    } else if(type == HyperEdge::EDGE_ROOT) {
        GetLeftHyp()->PrintParse(out);
    } else {
        out << "("<<(char)type<<" ";
        GetLeftHyp()->PrintParse(out);
        out << " ";
        GetRightHyp()->PrintParse(out);
        out << ")";
    }
}

void Hypothesis::PrintParse(const vector<string> & strs, ostream & out) const {
    HyperEdge::Type type = GetEdgeType();
    if(IsTerminal()) {
        out << "(" << (char)type;
        for(int i = GetLeft(); i <= GetRight(); i++)
            out << " ("<<(char)type<< "W " << GetTokenWord(strs[i]) <<")";
        out << ")";
    } else if(type == HyperEdge::EDGE_ROOT) {
        GetLeftHyp()->PrintParse(strs, out);
    } else {
        out << "("<<(char)type<<" ";
        GetLeftHyp()->PrintParse(strs, out);
        out << " ";
        GetRightHyp()->PrintParse(strs, out);
        out << ")";
    }
}

Hypothesis * Hypothesis::GetLeftHyp() const{
	if (!left_child_ || left_rank_ >= left_child_->GetHypotheses().size())
		return NULL;
	return left_child_->GetHypothesis(left_rank_);
}

Hypothesis * Hypothesis::GetRightHyp() const{
	if (!right_child_ || right_rank_ >= right_child_->GetHypotheses().size())
		return NULL;
	return right_child_->GetHypothesis(right_rank_);
}

void Hypothesis::GetReordering(std::vector<int> & reord, bool verbose) const{
	HyperEdge::Type type = this->GetEdgeType();
	if (verbose && !this->IsTerminal()){
		cerr << "Loss:" << this->GetLoss();
		const DiscontinuousHypothesis* dhyp =
				dynamic_cast<const DiscontinuousHypothesis*>(this);
		if (dhyp)
			cerr << *dhyp;
		else
			cerr << *this;
		this->PrintChildren(cerr);
	}
	if(type == HyperEdge::EDGE_FOR) {
		for(int i = this->GetLeft(); i <= this->GetRight(); i++)
			reord.push_back(i);
	} else if(type == HyperEdge::EDGE_BAC) {
		for(int i = this->GetRight(); i >= this->GetLeft(); i--)
			reord.push_back(i);
	} else if(type == HyperEdge::EDGE_ROOT) {
		GetLeftHyp()->GetReordering(reord, verbose);
	} else if(type == HyperEdge::EDGE_STR) {
		GetLeftHyp()->GetReordering(reord, verbose);
		GetRightHyp()->GetReordering(reord, verbose);
	} else if(type == HyperEdge::EDGE_INV) {
		GetRightHyp()->GetReordering(reord, verbose);
		GetLeftHyp()->GetReordering(reord, verbose);
	}
}
// Add up the loss over an entire subtree defined by span of this hypothesis
double Hypothesis::AccumulateLoss() {
    double score = GetLoss();
    if(GetLeftHyp())  score += GetLeftHyp()->AccumulateLoss();
    if(GetRightHyp())  score += GetRightHyp()->AccumulateLoss();
    return score;
}

// Clone a hypothesis with a hyper edge, which could be discontinuous
Hypothesis *Hypothesis::Clone() const
{
    Hypothesis *new_hyp;
    HyperEdge *edge = this->GetEdge();
    DiscontinuousHyperEdge *dedge = dynamic_cast<DiscontinuousHyperEdge*>(edge);
    // discontinuous + discontinuous = continuous
    if(dedge){
        new_hyp = new Hypothesis(*this);
        new_hyp->SetEdge(dedge->Clone());
    }
    // continuous + continuous = continuous
    else
    {
        new_hyp = new Hypothesis(*this);
        new_hyp->SetEdge(edge->Clone());
    }
    return new_hyp;
}

// We can skip this hypothesis in search
// if this hypothesis produce an ITG permutation from discontinuous children
bool Hypothesis::CanSkip(const HyperEdge * edge,
		const Hypothesis * left, const Hypothesis * right) {
	const DiscontinuousHypothesis * dleft =
			dynamic_cast<const DiscontinuousHypothesis*>(left);
	const DiscontinuousHypothesis * dright =
			dynamic_cast<const DiscontinuousHypothesis*>(right);
	// discontinuous + discotinuous = continuous
	return dleft && dright
			&& (edge->GetType() == dleft->GetEdgeType()
			|| edge->GetType() == dright->GetEdgeType()
			|| dleft->GetEdgeType() != dright->GetEdgeType());
}
