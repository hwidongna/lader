
AM_CPPFLAGS = -I$(srcdir)/../include $(BOOST_CPPFLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)


bin_PROGRAMS = train-lader lader evaluate-lader label-ranking \
				train-shift-reduce shift-reduce shift-reduce-kbest \
				extract-feature gold-standard iparser-gold train-iparser \
				renumber-feature reranker reranker-oracle iparser evaluate-iparser

train_lader_SOURCES = train-lader.cc
train_lader_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

lader_SOURCES = lader.cc
lader_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

evaluate_lader_SOURCES = evaluate-lader.cc
evaluate_lader_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

label_ranking_SOURCES = label-ranking.cc
label_ranking_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

train_shift_reduce_SOURCES = train-shift-reduce.cc
train_shift_reduce_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

shift_reduce_SOURCES = shift-reduce.cc
shift_reduce_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

shift_reduce_kbest_SOURCES = shift-reduce-kbest.cc
shift_reduce_kbest_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

extract_feature_SOURCES = extract-feature.cc
extract_feature_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

gold_standard_SOURCES = gold-standard.cc
gold_standard_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

train_iparser_SOURCES = train-iparser.cc
train_iparser_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

iparser_SOURCES = iparser.cc
iparser_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

evaluate_iparser_SOURCES = evaluate-iparser.cc
evaluate_iparser_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

iparser_gold_SOURCES = iparser-gold.cc
iparser_gold_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

renumber_feature_SOURCES = renumber-feature.cc
renumber_feature_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

reranker_SOURCES = reranker.cc
reranker_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt

reranker_oracle_SOURCES = reranker-oracle.cc
reranker_oracle_LDADD = ../lib/liblader.la -lboost_regex-mt -lboost_thread-mt -lboost_filesystem -lboost_system -lrt