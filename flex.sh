#!/bin/bash
cd $(dirname $0)
cd src/include/reranker
flex -oread-tree.h read-tree.l
