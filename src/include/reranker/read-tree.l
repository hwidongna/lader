/* read-tree.l
 *
 *  Created on: Feb 19, 2014
 *      Author: leona
 */

%option noyywrap
%option fast
%option prefix="readtree"
%option 8bit

%{

#ifndef READ_TREE_H_
#define READ_TREE_H_

#include "reranker/flat-tree.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stack>

using namespace reranker;

#define YY_DECL static GenericNode* readtree_lex()
%}

/* ROOT  - PTB-style tree root with no label
 * CHILD  - child subtree or terminal
 */

%s ROOT
%s CHILD

%%

%{
  GenericNode * root;    /* tree's root node */
  std::stack<GenericNode*> s;  /* stack of tree node ptrs */
  int i = 0;
%}

<ROOT>"("[SIF]		{
					assert(s.empty()); 
					root = new GenericNode(readtreetext[1]);
					s.push(root);
					BEGIN(CHILD);
					}
<ROOT><<EOF>>		assert(s.empty()); return NULL;

<CHILD>"("[SIF]		{
					assert(!s.empty()); 
					s.push(new GenericNode(readtreetext[1]));
					BEGIN(CHILD); 
					}
					
<CHILD>[^ \t\n()]+	{
					assert(!s.empty());
					s.top()->SetLeft(i++);
					s.top()->SetRight(i);
					BEGIN(CHILD);
					}
					
<CHILD>")"			{
					assert(!s.empty());
					GenericNode * child = s.top();
					if (!child->IsTerminal()){
						child->SetLeft(child->GetChildren().front()->GetLeft());
						child->SetRight(child->GetChildren().back()->GetRight()); 
					}
					s.pop();
					if (s.empty())
						return root;
					s.top()->AddChild(child);
					BEGIN(CHILD);
					}

[ \t]+				/* ignore spaces */
.					std::cerr << "Unexpected character: " <<  readtreetext << endl;

%%

GenericNode * ParseInput(const string & line)
{
	BEGIN(ROOT);
	YY_BUFFER_STATE buf = readtree_scan_string(line.c_str());
	GenericNode * tree = readtree_lex();
	readtree_delete_buffer(buf);
	return tree;
}

#endif