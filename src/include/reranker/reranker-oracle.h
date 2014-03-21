#ifndef REORDERER_EVALUATOR_H__ 
#define REORDERER_EVALUATOR_H__

#include <iostream>
#include <fstream>
#include <lader/config-base.h>
#include <lader/combined-alignment.h>
#include <lader/feature-data-sequence.h>
#include <lader/loss-chunk.h>
#include <lader/loss-tau.h>
#include <lader/ranks.h>
#include <boost/algorithm/string.hpp>
#include <reranker/flat-tree.h>
#include <shift-reduce-dp/dparser.h>
#include <sstream>

using namespace lader;
using namespace std;
using namespace boost;

namespace reranker {

// A class to train the reordering model
class RerankerOracle {
public:

    RerankerOracle() : attach_(CombinedAlign::ATTACH_NULL_LEFT),
        combine_(CombinedAlign::COMBINE_BLOCKS),
        bracket_(CombinedAlign::ALIGN_BRACKET_SPANS)
         { }
    ~RerankerOracle() { }
    
    // Run the evaluator
    void Evaluate(const ConfigBase & config){
        // Set the attachment handler
        attach_ = config.GetString("attach_null") == "left"
            ? CombinedAlign::ATTACH_NULL_LEFT : CombinedAlign::ATTACH_NULL_RIGHT;
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
        // Set up the pairs to store the oracle counts
        vector<pair<double,double> > oracle(losses.size(), pair<double,double>(0,0));
        // Open the files
        const vector<string> & args = config.GetMainArgs();
        ifstream aligns_in(SafeAccess(args, 0).c_str());
        if(!aligns_in) THROW_ERROR("Couldn't find alignment file " << args[0]);
        ifstream *src_in = NULL, *trg_in = NULL;
        if(args.size() > 1) {
            src_in = new ifstream(SafeAccess(args, 1).c_str());
            if(!(*src_in)) THROW_ERROR("Couldn't find source file " << args[1]);
        }
        if(args.size() > 2) {
            trg_in = new ifstream(SafeAccess(args, 2).c_str());
            if(!*trg_in) THROW_ERROR("Couldn't find target file " << args[2]);
        }

        string kbest_in = config.GetString("kbest_in");
        ifstream kin(kbest_in.c_str());
        if (!kin)
        	cerr << "use stdin for kbest_in" << endl;
        cerr << "Sentence Length #Parse Chunk RankChunk Tau RankTau" << endl;
        string data, align, src, trg;
        // Read them one-by-one and run the evaluator
        for (int sent = 0; getline(kin ? kin : cin, data) && getline(aligns_in, align) ; sent++) {
        	if(sent && sent % 1000 == 0) cerr << ".";
			if(sent && sent % (1000*10) == 0) cerr << sent << endl;
			// Get the input values
        	int numParses;
			istringstream iss(data);
			iss >> numParses;
            // Get the source file
            getline(*src_in, src);
            vector<string> srcs;
            algorithm::split(srcs, src, is_any_of(" "));
			FeatureDataSequence words;
			words.FromString(src);
            vector<GenericNode*> parses(numParses, NULL);
            for (int k = 0 ; k < numParses ; k++) {
            	getline(kin ? kin : cin, data);
            	if (data.empty())
            		THROW_ERROR("Less than " << numParses << " trees" << endl);
            	// Load the data
            	vector<string> columns;
            	algorithm::split(columns, data, is_any_of("\t"));
            	if (columns.size() != 2)
            		THROW_ERROR("Expect score<TAB>kbest, get: " << data << endl);
            	// Load the data
            	double score = atof(columns[0].c_str());
            	ActionVector refseq = DPState::ActionFromString(columns[1].c_str());
				Parser * p = new DParser(words.GetNumWords()); // allow the largest number of swap actions
				DPState * goal = p->GuidedSearch(refseq, words.GetNumWords());
				if (goal == NULL)
					THROW_ERROR(k << "th best " << columns[1].c_str() << endl
							<< "Fail to produce the guided search result" << endl)
				parses[k] = goal->ToFlatTree();
            	delete p;
            }
            // Get the ranks
            Ranks ranks = Ranks(CombinedAlign(srcs,
                                              Alignment::FromString(align),
                                              attach_, combine_, bracket_));
            // Compute losses of all parses and keep the best parse
            vector<pair<double,double> > minloss(losses.size(), pair<double,double>(0,0));
            vector<double> maxacc(losses.size(), 0);
            vector<int> maxrank(losses.size(), 0);
            for (int k = 0; k < parses.size() ; k++){
            	GenericNode * p = parses[k];
            	if (!p)
            		continue;
            	// Print the input values
            	std::vector<int> order;
            	p->GetOrder(order);
            	for(int i = 0; i < (int) losses.size(); i++) {
            		pair<double,double> my_loss =
            				losses[i]->CalculateSentenceLoss(order,&ranks,NULL);
            		double acc = my_loss.second == 0 ?
            				1 : (1-my_loss.first/my_loss.second);
            		if (acc > maxacc[i]){
            			maxacc[i] = acc;
						minloss[i].first = my_loss.first;
						minloss[i].second = my_loss.second;
						maxrank[i] = k;
            		}
            	}
            }
            cerr << sent << "\t" << words.GetNumWords() << "\t" << numParses;
            // best parse has the minimum loss
        	for(int i = 0; i < (int) losses.size(); i++) {
        		oracle[i].first += minloss[i].first;
        		oracle[i].second += minloss[i].second;
				cerr << "\t" << maxacc[i] << "\t" << maxrank[i];
        	}
        	cerr << endl;
            // Print the reference reordering
            vector<vector<string> > src_order(ranks.GetMaxRank()+1);
            for(int i = 0; i < (int)srcs.size(); i++)
                src_order[ranks[i]].push_back(SafeAccess(srcs,i));
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
            // If source and target files exist, print them as well
            cout << "src:\t" << src << endl;
            if(args.size() > 3) {
                getline(*trg_in, trg);
                cout << "trg:\t" << trg << endl;
            }
            for (int k = 0; k < parses.size() ; k++){
				GenericNode * p = parses[k];
				if (!p)
					continue;
				// Print the input values
				cout << "sys:\t";
				p->PrintString(words.GetSequence(), cout);
				cout << endl;
				std::vector<int> order;
				p->GetOrder(order);
				for(int i = 0; i < (int) losses.size(); i++) {
					pair<double,double> my_loss =
							losses[i]->CalculateSentenceLoss(order,&ranks,NULL);
					double acc = my_loss.second == 0 ?
							1 : (1-my_loss.first/my_loss.second);
					if(i != 0) cout << "\t";
					if(k == maxrank[i]) cout << "*";
					cout << losses[i]->GetName() << "=" << acc
							<< " (loss "<<my_loss.first<< "/" <<my_loss.second<<")";
				}
				cout << endl ;
				delete p;
			}
            cout << endl;
        }
        cout << "Total:" << endl;
        for(int i = 0; i < (int) oracle.size(); i++) {
            if(i != 0) cout << "\t";
            cout << losses[i]->GetName() << "="
                 << (1 - oracle[i].first/oracle[i].second)
                 << " (loss "<<oracle[i].first << "/" <<oracle[i].second<<")";
        }
        cout << endl;
    }

private:
    CombinedAlign::NullHandler attach_;
    CombinedAlign::BlockHandler combine_;
    CombinedAlign::BracketHandler bracket_;

};

}

#endif

