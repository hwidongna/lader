#include <lader/reorderer-evaluator.h>
#include <lader/feature-data-sequence.h>
#include <lader/loss-chunk.h>
#include <lader/loss-tau.h>
#include <lader/ranks.h>
#include <boost/algorithm/string.hpp>
#include <sstream>

using namespace lader;
using namespace std;
using namespace boost;

// Run the evaluator
void ReordererEvaluator::Evaluate(const ConfigEvaluator & config) {
    // Set the attachment handler
    attach_ = config.GetString("attach_null") == "left" ?
    		CombinedAlign::ATTACH_NULL_LEFT :
    		CombinedAlign::ATTACH_NULL_RIGHT;
    combine_ = config.GetBool("combine_blocks") ? 
            CombinedAlign::COMBINE_BLOCKS :
            CombinedAlign::LEAVE_BLOCKS_AS_IS;
    bracket_ = config.GetBool("combine_brackets") ?
			CombinedAlign::ALIGN_BRACKET_SPANS :
			CombinedAlign::LEAVE_BRACKETS_AS_IS;
    // Set up the losses
    vector<LossBase*> losses;
    losses.push_back(new LossChunk());
    losses.push_back(new LossTau());
    // Set up the pairs to store the counts
    vector<pair<double,double> > sums(losses.size(), pair<double,double>(0,0));
    // Open the files
    const vector<string> & args = config.GetMainArgs();
    ifstream aligns_in(SafeAccess(args, 0).c_str());
    if(!aligns_in) THROW_ERROR("Couldn't find alignment file " << args[0]);
    ifstream hyp_in(SafeAccess(args, 1).c_str());
    if(!hyp_in) THROW_ERROR("Couldn't find hypothesis file " << args[1]);
    ifstream src_in, trg_in;
    if(args.size() > 2) {
        src_in.open(SafeAccess(args, 2).c_str(), ios_base::in);
        if(!src_in) THROW_ERROR("Couldn't find source file " << args[2]);
    }
    if(args.size() > 3) {
    	trg_in.open(SafeAccess(args, 3).c_str(), ios_base::in);
        if(!trg_in) THROW_ERROR("Couldn't find target file " << args[3]);
    }
    string hline, aline, sline, tline;
    // Read them one-by-one and run the evaluator
    while(getline(hyp_in, hline) && getline(aligns_in, aline)) {
        // Get the input values
        std::vector<string> datas;
        algorithm::split(datas, hline, is_any_of("\t"));
        istringstream iss(datas[0]);
        std::vector<int> order; int ival;
        while(iss >> ival) order.push_back(ival);
        // Get the source file
        getline(src_in, sline);
        FeatureDataSequence srcs;
        srcs.FromString(sline);
        // Get the ranks
        // a vector contains lists of source indices by rank
        Ranks ranks(CombinedAlign(srcs.GetSequence(), Alignment::FromString(aline), attach_, combine_, bracket_));
        // in case of utilizing bilingual features
        if(args.size() > 3) {
			vector<vector<int> > ranked_vec(ranks.GetMaxRank() + 1);
			for(int j = 0; j < (int)srcs.GetNumWords(); j++)
				ranked_vec[ranks[j]].push_back(j);
			// obtain the new order of the source sentence
			vector<int> new_order;
			BOOST_FOREACH(vector<int> & ranked, ranked_vec)
				new_order.insert(new_order.end(), ranked.begin(), ranked.end());
			// the rank is the new order
			ranks.SetRanks(new_order);
			// reorder the original source sentence
			srcs.Reorder(new_order);
        }
        // Print the input values
        cout << "hyp_ord:\t" << datas[0] << endl;
        for(int i = 1; i < (int)datas.size(); i++)
            cout << "hyp_"<<i<<":\t" << datas[i] << endl;
        // If source and target files exist, print them as well
        cout << "src:\t" << srcs.ToString() << endl;
        cout << "ord:\t";
        for(int i = 0; i < ranks.GetSrcLen() ; i++){
        	if (i > 0) cout << " ";
        	cout << ranks[i];
        }
        cout << endl;
        // Print the reference reordering
        vector<vector<string> > src_order(ranks.GetMaxRank()+1);
        for(int i = 0; i < srcs.GetNumWords(); i++)
            src_order[ranks[i]].push_back(srcs.GetElement(i));
        cout << "ref:\t";
        for(int i = 0; i < (int)src_order.size(); i++) {
            if(i != 0) cout << " ";
            // If there is only one, print the string
            if(src_order[i].size() == 1) {
                cout << src_order[i][0];
            // If there is more than one, print a bracketed group
            } else {
                cout << "{{";
                BOOST_FOREACH(const string & s, src_order[i])
                    cout << " " << s;
                cout << " }}";
            }
        }
        cout << endl;
        if(args.size() > 3) {
            getline(trg_in, tline);
            cout << "trg:\t" << tline << endl;
        }
        // Score the values
        for(int i = 0; i < (int) losses.size(); i++) {
            pair<double,double> my_loss = 
                    losses[i]->CalculateSentenceLoss(order,&ranks,NULL);
            sums[i].first += my_loss.first;
            sums[i].second += my_loss.second;
            double acc = my_loss.second == 0 ? 
                                    1 : (1-my_loss.first/my_loss.second);
            if(i != 0) cout << "\t";
            cout << losses[i]->GetName() << "=" << acc 
                 << " (loss "<<my_loss.first<< "/" <<my_loss.second<<")";
        }
        cout << endl << endl;
    }
    cout << "Total:" << endl;
    for(int i = 0; i < (int) sums.size(); i++) {
        if(i != 0) cout << "\t";
        cout << losses[i]->GetName() << "="
             << (1 - sums[i].first/sums[i].second)
             << " (loss "<<sums[i].first << "/" <<sums[i].second<<")";
    }
    cout << endl;
}
