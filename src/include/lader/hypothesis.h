#ifndef HYPOTHESIS_H__ 
#define HYPOTHESIS_H__

#include <lader/hyper-edge.h>
#include <lader/feature-vector.h>
#include <lader/util.h>
#include <lader/hypothesis-queue.h>
#include <lader/feature-data-base.h>
#include <lader/reorderer-model.h>
#include <sstream>
#include <iostream>
#include <vector>
#include "lm/model.hh"
using namespace std;

namespace std {
inline std::ostream& operator <<(std::ostream& out,
		const lader::Hypothesis & rhs);
}
namespace lader {

typedef std::tr1::unordered_map<const HyperEdge*, FeatureVectorInt*,
		PointerHash<const HyperEdge*>, PointerEqual<const HyperEdge*> > EdgeFeatureMap;
typedef std::pair<const HyperEdge*, FeatureVectorInt*> EdgeFeaturePair;

class TargetSpan;
class FeatureSet;

// A tuple that is used to hold hypotheses during cube pruning
class Hypothesis {
public:
	Hypothesis(double viterbi_score, double single_score, double non_local_score,
			HyperEdge * edge,
			int trg_left, int trg_right,
			lm::ngram::State state,
			int left_rank = -1, int right_rank = -1,
			TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
				viterbi_score_(viterbi_score), single_score_(single_score),
				non_local_score_(non_local_score), loss_(0),
				edge_(edge),
				trg_left_(trg_left), trg_right_(trg_right),
				state_(state),
				left_child_(left_child), right_child_(right_child),
				left_rank_(left_rank), right_rank_(right_rank) {}

protected:
	friend class TestHyperGraph;
	friend class TestDiscontinuousHyperGraph;
	// just for testing
	Hypothesis(double viterbi_score, double single_score, double non_local_score,
			HyperEdge * edge,
			int trg_left, int trg_right,
			int left_rank = -1, int right_rank = -1,
			TargetSpan* left_child = NULL, TargetSpan* right_child = NULL) :
				viterbi_score_(viterbi_score), single_score_(single_score),
				non_local_score_(non_local_score), loss_(0),
				edge_(edge),
				trg_left_(trg_left), trg_right_(trg_right),
				state_(lm::ngram::State()),
				left_child_(left_child), right_child_(right_child),
				left_rank_(left_rank), right_rank_(right_rank) {}

	// just for testing
	Hypothesis(double viterbi_score, double single_score, double non_local_score,
			int left, int right,
			int trg_left, int trg_right,
			HyperEdge::Type type, int center = -1,
			int left_rank = -1, int right_rank = -1,
			TargetSpan *left_child =NULL, TargetSpan *right_child = NULL) :
			viterbi_score_(viterbi_score), single_score_(single_score),
			non_local_score_(non_local_score), loss_(0),
			edge_(new HyperEdge(left, center, right, type)),
			trg_left_(trg_left), trg_right_(trg_right),
			left_child_(left_child), right_child_(right_child),
			left_rank_(left_rank), right_rank_(right_rank) {}

public:
	virtual ~Hypothesis() {
		if (edge_ != NULL)
			delete edge_;
	}

	std::string GetRuleString(const std::vector<std::string> & sent,
			char left_val = 0, char right_val = 0) const {
		std::ostringstream ret;
		ret << "[" << (char) (((GetEdgeType()))) << "] |||";
		if (left_val) {
			ret << " [" << left_val << "]";
			if (right_val)
				ret << " [" << right_val << "]";

		} else {
			for (int i = GetLeft(); i <= GetRight(); i++) {
				ret << " ";
				for (int j = 0; j < (int) (((sent[i].length()))); j++) {
					if (sent[i][j] == '\\' || sent[i][j] == '\"')
						ret << "\\";

					ret << sent[i][j];
				}
			}

		}

		return ret.str();
	}

	virtual bool operator <(const Hypothesis & rhs) const {
		// just compare viterbi_score
		return viterbi_score_ < rhs.viterbi_score_;
	}

	virtual bool operator ==(const Hypothesis & rhs) const {
		// a terminal hypothesis is alwasy unique
		if (IsTerminal() || rhs.IsTerminal()
		|| GetLeft() != rhs.GetLeft() || GetRight() != rhs.GetRight()
		|| trg_left_ != rhs.trg_left_ || trg_right_ != rhs.trg_right_)
			return false;
		// compare the permutation
		vector<int> this_reorder;
		vector<int> rhs_reorder;
		GetReordering(this_reorder);
		rhs.GetReordering(rhs_reorder);
		if (this_reorder.size() != rhs_reorder.size())
			THROW_ERROR("Difference size of hypothesis" << endl << *this << endl << rhs)
		return std::equal(this_reorder.begin(), this_reorder.end(),
						rhs_reorder.begin());
	}

	void GetReordering(std::vector<int> & reord, bool verbose = false) const;

	double GetScore() const {
		return viterbi_score_;
	}

	double GetSingleScore() const {
		return single_score_;
	}

	double GetNonLocalScore() const {
		return non_local_score_;
	}

	double GetLoss() const {
		return loss_;
	}

	HyperEdge *GetEdge() const {
		return edge_;
	}

	int GetLeft() const {
		return edge_->GetLeft();
	}

	int GetRight() const {
		return edge_->GetRight();
	}

	int GetTrgLeft() const {
		return trg_left_;
	}

	int GetTrgRight() const {
		return trg_right_;
	}

	int GetCenter() const {
		return edge_->GetCenter();
	}

	HyperEdge::Type GetEdgeType() const {
		return edge_->GetType();
	}

	TargetSpan *GetLeftChild() const {
		return left_child_;
	}

	TargetSpan *GetRightChild() const {
		return right_child_;
	}

	int GetLeftRank() const {
		return left_rank_;
	}

	int GetRightRank() const {
		return right_rank_;
	}

	Hypothesis *GetLeftHyp() const;
	Hypothesis *GetRightHyp() const;
	lm::ngram::Model::State GetState() const {
		return state_;
	}

	void SetScore(double dub) {
		viterbi_score_ = dub;
	}

	void SetSingleScore(double dub) {
		single_score_ = dub;
	}

	void SetNonLocalScore(double dub) {
		non_local_score_ = dub;
	}

	void SetLoss(double dub) {
		loss_ = dub;
	}

	void SetEdge(HyperEdge *edge) {
		edge_ = edge;
	}

	void SetLeftChild(TargetSpan *dub) {
		left_child_ = dub;
	}

	void SetRightChild(TargetSpan *dub) {
		right_child_ = dub;
	}

	void SetLeftRank(int dub) {
		left_rank_ = dub;
	}

	void SetRightRank(int dub) {
		right_rank_ = dub;
	}

	void SetTrgLeft(int dub) {
		trg_left_ = dub;
	}

	void SetTrgRight(int dub) {
		trg_right_ = dub;
	}

	void SetType(HyperEdge::Type type) {
		edge_->SetType(type);
	}

	void SetState(lm::ngram::State out){
		state_ = out;
	}

	virtual void PrintChildren(std::ostream & out) const;
	void PrintParse(const vector<string> & strs, ostream & out) const;
	double AccumulateLoss();
//	FeatureVectorInt AccumulateFeatures(const EdgeFeatureMap *features);
//	FeatureVectorInt AccumulateFeatures(
//			std::tr1::unordered_map<int, double> & feat_map
//			, const EdgeFeatureMap *features);
	bool IsTerminal() const {
		return GetEdgeType() == HyperEdge::EDGE_FOR
				|| GetEdgeType() == HyperEdge::EDGE_BAC;
	}

	virtual Hypothesis *Clone() const;
	virtual bool CanSkip(int max_seq = 0);
private:
//	void AccumulateFeatures(const EdgeFeatureMap *features,
//			std::tr1::unordered_map<int, double> & feat_map);
	double viterbi_score_;
	double single_score_;
	double non_local_score_;
	double loss_;
	HyperEdge *edge_;
	int trg_left_, trg_right_;
	TargetSpan *left_child_, *right_child_;
	int left_rank_, right_rank_;
	lm::ngram::Model::State state_;
};

}

namespace std {
// Output function for pairs
inline std::ostream& operator <<(std::ostream& out,
		const lader::Hypothesis & rhs) {
	out << "<" << rhs.GetLeft() << ", " << rhs.GetRight() << ", "
			<< rhs.GetTrgLeft() << ", " << rhs.GetTrgRight() << ", "
			<< (char) rhs.GetEdgeType() << (char) rhs.GetEdge()->GetClass() << ", "
			<< rhs.GetCenter() << " :: "
			<< rhs.GetScore() << ", " << rhs.GetSingleScore() << ", "
			<< rhs.GetNonLocalScore() << ">";
	vector<int> reorder;
	rhs.GetReordering(reorder);
	BOOST_FOREACH(int i, reorder)
		out << " " << i;
	return out;
}
}

#endif

