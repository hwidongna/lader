
#include <lader/feature-base.h>
#include <lader/feature-sequence.h>
#include <lader/feature-parse.h>
#include <lader/feature-align.h>
#include <lader/discontinuous-hyper-edge.h>

using namespace lader;
using namespace std;

FeatureBase * FeatureBase::CreateNew(const string & type) {
    // Create a new feature base based on the type
    if(type == "seq")
        return new FeatureSequence;
    else if(type == "cfg")
        return new FeatureParse;
    else if(type == "align")
        return new FeatureAlign;
    else
        THROW_ERROR("Bad feature type " << type << " (must be seq/cfg/align)");
    return NULL;
}


int FeatureBase::GetBalance(const HyperEdge & edge)
{
	int bal;
    // Get the balance between the values
    if(edge.GetClass() == 'D'){

    	const DiscontinuousHyperEdge * e =
    			dynamic_cast<const DiscontinuousHyperEdge*>(&edge);
        // continuous + continuous = discontinuous
    	if (edge.GetCenter() < 0){
        	return e->GetRight() - e->GetN() - e->GetM() + e->GetLeft();
        }
        // continuous + discontinuous = discontinuous
        else if (edge.GetCenter() <= e->GetM()){
        	return e->GetRight() - e->GetN() + e->GetM() - 2 * e->GetCenter() + e->GetLeft() + 2;
        }
        // discontinuous + continuous = discontinuous
        else if (edge.GetCenter() > e->GetN()){
        	return e->GetRight() + e->GetN() - e->GetM() - 2 * e->GetCenter() + e->GetLeft();
		}
    	// discontinuous + discontinuous = continuous
        else if (e->GetM() < edge.GetCenter() && edge.GetCenter() < e->GetN()){
        	return e->GetRight() - 2 * e->GetN() - 2 * e->GetM() + 2 * e->GetCenter() + e->GetLeft() + 1;
        }
        else
        	THROW_ERROR("Balance is undefined for " << *e);
    }
    return edge.GetRight() - 2 * edge.GetCenter() + edge.GetLeft() + 1;
}

int FeatureBase::GetSpanSize(const HyperEdge & edge)
{
	int bal;
    // Get the balance between the values
    if(edge.GetClass() == 'D' ){
        DiscontinuousHyperEdge *e = (DiscontinuousHyperEdge*)(&edge);
        // continuous + continuous = discontinuous
        // continuous + discontinuous = discontinuous
        // discontinuous + continuous = discontinuous
        if (edge.GetCenter() < 0 || edge.GetCenter() <= e->GetM() || edge.GetCenter() > e->GetN())
            return (e->GetRight() - e->GetN() + 1) + (e->GetM() - e->GetLeft() + 1);
    }
	// discontinuous + discontinuous = continuous
    return edge.GetRight() - edge.GetLeft() + 1;
}
