/*
 * reorderer-oracle.cc
 *
 *  Created on: Sep 5, 2013
 *      Author: leona
 */

#include "reorderer-oracle.h"
#include <lader/ranks.h>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <fstream>

using namespace lader;
using namespace std;
using namespace boost;

namespace lader {
// Run the evaluator
void ReordererOracle::Oracle(const ConfigOracle & config) {
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
    // Open the files
    const vector<string> & args = config.GetMainArgs();
    ifstream aligns_in(SafeAccess(args, 0).c_str());
    if(!aligns_in) THROW_ERROR("Couldn't find alignment file " << args[0]);
    ifstream src_in(SafeAccess(args, 1).c_str());
    if(!src_in) THROW_ERROR("Couldn't find input file " << args[1]);
    string data, align, src, trg;
    // Read them one-by-one and run the evaluator
    while(getline(src_in, src) && getline(aligns_in, align)) {
        vector<string> srcs;
        algorithm::split(srcs, src, is_any_of(" "));
        // Get the ranks
        Ranks ranks = Ranks(CombinedAlign(srcs,
                                          Alignment::FromString(align),
                                          attach_, combine_, bracket_));
        // Print the reference reordering
        vector<vector<string> > src_order(ranks.GetMaxRank()+1);
        for(int i = 0; i < (int)srcs.size(); i++)
            src_order[ranks[i]].push_back(SafeAccess(srcs,i));
        for(int i = 0; i < (int)src_order.size(); i++) {
            if(i != 0) cout << " ";
            if(src_order[i].size() == 1)
                cout << src_order[i][0];
            else
            	cout << algorithm::join(src_order[i], " ");
        }
        cout << endl;
    }
}

} /* namespace lader */
