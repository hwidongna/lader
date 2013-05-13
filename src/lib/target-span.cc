#include <lader/target-span.h>
#include <sstream>

using namespace lader;
using namespace std;

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


void TargetSpan::PrintParse(const vector<string> & strs, ostream & out) const {
    HyperEdge::Type type = hyps_[0]->GetEdgeType();
    if(type == HyperEdge::EDGE_FOR || type == HyperEdge::EDGE_BAC) {
        out << "(" << (char)type;
        for(int i = left_; i <= right_; i++)
            out << " ("<<(char)type<< "W " << GetTokenWord(strs[i]) <<")";
        out << ")";
    } else if(type == HyperEdge::EDGE_ROOT) {
        hyps_[0]->GetLeftChild()->PrintParse(strs, out);
    } else {
        out << "("<<(char)type<<" ";
        hyps_[0]->GetLeftChild()->PrintParse(strs, out);
        out << " ";
        hyps_[0]->GetRightChild()->PrintParse(strs, out);
        out << ")";
    }
}

void TargetSpan::GetReordering(std::vector<int> & reord) const{
    HyperEdge::Type type = hyps_[0]->GetEdgeType();
    if(type == HyperEdge::EDGE_FOR) {
        for(int i = left_; i <= right_; i++)
            reord.push_back(i);
    } else if(type == HyperEdge::EDGE_BAC) {
        for(int i = right_; i >= left_; i--)
            reord.push_back(i);
    } else if(type == HyperEdge::EDGE_ROOT) {
        hyps_[0]->GetLeftChild()->GetReordering(reord);
    } else if(type == HyperEdge::EDGE_STR) {
        hyps_[0]->GetLeftChild()->GetReordering(reord);
        hyps_[0]->GetRightChild()->GetReordering(reord);
    } else if(type == HyperEdge::EDGE_INV) {
        hyps_[0]->GetRightChild()->GetReordering(reord);
        hyps_[0]->GetLeftChild()->GetReordering(reord);
    }
}

void TargetSpan::LabelWithIds(int & curr_id){
	if(id_ != -1) return;
	BOOST_FOREACH(Hypothesis * hyp, hyps_) {
		if(hyp->GetLeftChild())
			hyp->GetLeftChild()->LabelWithIds(curr_id);
		if(hyp->GetRightChild())
			hyp->GetRightChild()->LabelWithIds(curr_id);
	}
	id_ = curr_id++;
}
