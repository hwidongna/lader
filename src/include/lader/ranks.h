#ifndef RANKS_H__ 
#define RANKS_H__

#include <vector>
#include <lader/combined-alignment.h>
#include <shift-reduce-dp/dpstate.h>
namespace lader {

class Ranks {
public:
	typedef enum{
		INSERT_L = -2,
		INSERT_R = -1,
	}_;
    Ranks() : max_rank_(-1) { };
    // Turn a combined alignment into its corresponding ranks
    Ranks(const CombinedAlign & combined);
    virtual ~Ranks() {}
    static bool IsStepOneUp(int l, int r)
    {
        return l + 1 == r;
    }

    // Check whether two ranks are contiguous (IE same, or step one up)
    static bool IsContiguous(int l, int r)
    {
        return l == r || IsStepOneUp(l, r);
    }
    static Ranks FromString(const string & line){
    	Ranks rank;
    	istringstream iss(line);
		int i;
		while (iss >> i){
			rank.ranks_.push_back(i);
			if (rank.max_rank_ < i)
				rank.max_rank_ = i;
		}
		return rank;
    }
    // Access the rank
    int operator[](int i) const { return SafeAccess(ranks_, i); }
    int GetMaxRank() const { return max_rank_; }
    const std::vector<int> & GetRanks() const { return ranks_; }
    int GetSrcLen() const { return ranks_.size(); }
    void SetRanks(const std::vector<int> & order);
    ActionVector GetReference() const;
    bool IsStraight(DPState * leftstate, DPState * state) const;
    bool IsInverted(DPState * leftstate, DPState * state) const;
    bool HasTie(DPState * state) const;
    bool IsSwap(DPState * state) const;
    bool HasContinuous(DPState * state) const;
protected:
    std::vector<int> ranks_;
    int max_rank_;
};

}

#endif

