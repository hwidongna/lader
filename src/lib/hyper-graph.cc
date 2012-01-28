#include <kyldr/hyper-graph.h>

using namespace kyldr;
using namespace std;

HyperNode * HyperGraph::GetNodeAtSpan(const HyperSpan & span,
                                      bool add) {
    SpanNodeMap::iterator it = span_node_map_.find(span);
    // If the node is found, return it right away
    if(it != span_node_map_.end())
        return it->second;
    if(!add)
        return NULL;
    // If the node is not found, add it
    nodes_.push_back(new HyperNode(nodes_.size(), span));
    HyperNode * ret = *nodes_.rbegin();
    // Add to the map
    span_node_map_.insert(MakePair(span, ret));
    // If the node spans the entire source side, we need to add it to
    // the root as well
    if(span.GetLeft() == 0 && span.GetRight() == GetSrcLen() - 1) {
        HyperEdge * edge = AddNewEdge(HyperEdge::EDGE_ROOT, ret);
        nodes_[0]->AddEdge(edge);
    }
    return ret;
}

void HyperGraph::AddTerminals(const CombinedAlignment & combined,
                              int max_len,
                              bool use_inverse) {
    int len = combined.GetSrcLen();
    for(int i = 0; i < len; i++) {
        int end = min(len, i+max_len);
        for(int j = i; j < end; j++) {
            // Add the straight terminal
            HyperNode * str_node = 
                GetNodeAtSpan(HyperSpan(i,j,combined[i],combined[j]));
            HyperEdge * str_edge = AddNewEdge(HyperEdge::EDGE_TERMSTR);
            str_node->AddEdge(str_edge);
            // Add the inverse terminal
            if(use_inverse) {
                HyperNode * inv_node = 
                    GetNodeAtSpan(HyperSpan(i,j,combined[j],combined[i]));
                HyperEdge * inv_edge = AddNewEdge(HyperEdge::EDGE_TERMINV);
                inv_node->AddEdge(inv_edge);
            }
        }
    }
}

void HyperGraph::AddNonTerminalPair(HyperNode * left, HyperNode * right) {
    const HyperSpan & l = left->GetSpan(), & r = right->GetSpan();
    // Combine the straight non-terminal
    HyperNode * str_node = GetNodeAtSpan(
        HyperSpan(l.GetLeft(), r.GetRight(), l.GetTrgLeft(), r.GetTrgRight()));
    // cerr << "adding new node " << str_node->GetIdx() << ": " << str_node->GetSpan() << endl;
    str_node->AddEdge(AddNewEdge(HyperEdge::EDGE_STR,left,right));
    // Combine the inverted non-terminal (note l and r are switched)
    HyperNode * inv_node = GetNodeAtSpan(
        HyperSpan(l.GetLeft(), r.GetRight(), r.GetTrgLeft(), l.GetTrgRight()));
    inv_node->AddEdge(AddNewEdge(HyperEdge::EDGE_INV,left,right));
}

void HyperGraph::AddNonTerminals() {
    // Go through all the nodes
    for(int i = 1; i < (int)nodes_.size(); i++) {
        // Save the current values
        HyperNode * node = nodes_[i];
        // Travel through and combine all left neighbors
        const vector<HyperNode*> & left_neighbors
            = GetLeftNeighbors(node->GetSpan().GetLeft());
        for(int j = 0; j < (int)left_neighbors.size(); j++)
            AddNonTerminalPair(left_neighbors[j], node);
        // Travel through and combine all right neighbors
        const vector<HyperNode*> & right_neighbors
            = GetRightNeighbors(node->GetSpan().GetRight());
        for(int j = 0; j < (int)right_neighbors.size(); j++)
            AddNonTerminalPair(node, right_neighbors[j]);
        // Add the current span to the neighboring nodes
        int r = node->GetSpan().GetRight() + 1, 
            l = node->GetSpan().GetLeft() - 1;
        if(r != (int)left_neighbors_.size())
            SafeAccess(left_neighbors_, r).push_back(node);
        if(l != -1)
            SafeAccess(right_neighbors_, l).push_back(node);
    }
}

void HyperGraph::AccumulateFeatures(const HyperNode * node,
                        map<string,double> & feature_map) const{
    // Add node features
    BOOST_FOREACH(FeatureTuple feat_pair, SafeReference(GetNodeFeatures(*node)))
        feature_map[feat_pair.name] += feat_pair.value;
    // Add edge features
    const HyperEdge * edge = node->GetBestEdge();
    BOOST_FOREACH(FeatureTuple feat_pair, 
                SafeReference(GetEdgeFeatures(*edge)))
        feature_map[feat_pair.name] += feat_pair.value;
    // Recurse if necessary
    if(edge->GetLeftChild() != NULL)
        AccumulateFeatures(edge->GetLeftChild(), feature_map);
    if(edge->GetRightChild() != NULL)
        AccumulateFeatures(edge->GetRightChild(), feature_map);
}
