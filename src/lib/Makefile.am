LADERCPP = lader.cc hyper-graph.cc combined-alignment.cc alignment.cc \
           feature-sequence.cc feature-set.cc feature-base.cc \
           feature-vector.cc reorderer-model.cc loss-chunk.cc \
           reorderer-trainer.cc loss-base.cc reorderer-runner.cc \
           ranks.cc reorderer-evaluator.cc loss-tau.cc dictionary.cc \
           feature-data-parse.cc target-span.cc feature-parse.cc \
           loss-bracket.cc feature-align.cc thread-pool.cc hypothesis.cc

SRDPCPP = parser.cc dpstate.cc shift-reduce-trainer.cc ddpstate.cc flat-tree.cc \
		  idpstate.cc iparser-trainer.cc iparser-evaluator.cc iparser-ranks.cc

RERANKERCPP = reranker-model.cc

AM_CPPFLAGS = -I$(srcdir)/../include $(BOOST_CPPFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)


lib_LTLIBRARIES = liblader.la

liblader_la_SOURCES = ${LADERCPP} ${SRDPCPP} ${RERANKERCPP}
liblader_la_LDFLAGS = -version-info 0:0:0
