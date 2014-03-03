/*
 * renumber-feature.cc
 *
 *  Created on: Feb 27, 2014
 *      Author: leona
 */



#include <reranker/config-renumber.h>
#include <reranker/reranker-model.h>
#include <ctype.h>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <vector>
using namespace reranker;
using namespace std;
using namespace boost;

int main(int argc, char** argv) {
    // load the arguments
    ConfigRenumber config;
    vector<string> args = config.loadConfig(argc,argv);
    string model_in = config.GetString("model_in");
    RerankerModel * model;
    std::ifstream in(model_in.c_str());
    if(!in) THROW_ERROR("Could not open model file: " <<model_in);
    model = RerankerModel::FromStream(in, true);
    in.close();

    string line;
	string source_in = config.GetString("source_in");
	ifstream sin(source_in.c_str());
	if (!sin)
		cerr << "use stdin for source_in" << endl;
	getline(sin != NULL? sin : cin, line);
	cout << line << endl;
	int numSent = atoi(line.substr(2).c_str()); // "S="
	for (int sent = 0 ; sent < numSent ; sent++){
    	if(sent && sent % 1000 == 0) cerr << ".";
    	if(sent && sent % (1000*10) == 0) cerr << sent << endl;
		getline(sin != NULL? sin : cin, line);
		vector<string> parses;
		algorithm::split(parses, line, is_any_of(","));
		BOOST_FOREACH(string p, parses){
			vector<string> terms;
			algorithm::split(terms, p, is_any_of(" "));
			int i = 0;
			BOOST_FOREACH(string t, terms){
				if (i++ != 0) cout << " ";
				if (isdigit(t[0])){
					int idx;
					if (t.find("=") != t.npos )
						idx = atoi(t.substr(0, t.find("=")).c_str());
					else
						idx = atoi(t.c_str());
					int newidx = model->GetConvertion(idx);
					if (newidx >= 0){
						cout << newidx;
						if (t.find("=") != t.npos )
							cout << t.substr(t.find("="));
					}
				}else
					cout << t;
			}
			if (!p.empty())
				cout << ",";
		}
		cout << endl;
	}
	if (config.GetString("model_out").length()){
		string model_out = config.GetString("model_out");
		ofstream out(model_out.c_str());
		model->ToStream(out, config.GetInt("threshold"));
		out.close();
	}
	delete model;
}
