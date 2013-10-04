set -x

N=$1
D=$2
SAMPLES=$3
VERBOSE=$4
SHUFFLE=$5

ITERATION=500
BEAM=100

for d in $(seq 0 1 $D)
do
#	echo "" > train-model.$d
#	echo "" > test-model.$d
#    for i in $(seq 1 1 $N)
#    do
#        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE false false $ITERATION > train-model.$d.out 2> train-model.$d.log
#        tail -n2 train-model.$d.out >> train-model.$d
#        ./test-model.sh $d $BEAM $VERBOSE false false > test-model.$d.out 2> test-model.$d.log
#        tail -n1 test-model.$d.out >> test-model.$d
#        rm output/train-g$d.mod*
#    done
        
	echo "" > train-model-cube-growing.$d
	echo "" > test-model-cube-growing.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE true false $ITERATION > train-model-cube-growing.$d.out 2> train-model-cube-growing.$d.log
        tail -n2 train-model-cube-growing.$d.out >> train-model-cube-growing.$d
        ./test-model.sh $d $BEAM $VERBOSE true false > test-model-cube-growing.$d.out 2> test-model-cube-growing.$d.log
        tail -n1 test-model-cube-growing.$d.out >> test-model-cube-growing.$d
        rm output/train-g$d.mod*
    done

    echo "" > train-model-full-fledged.$d
	echo "" > test-model-full-fledged.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE true true $ITERATION > train-model-full-fledged.$d.out 2> train-model-full-fledged.$d.log
        tail -n2 train-model-full-fledged.$d.out >> train-model-full-fledged.$d
        ./test-model.sh $d $BEAM $VERBOSE true true > test-model-full-fledged.$d.out 2> test-model-full-fledged.$d.log
        tail -n1 test-model-full-fledged.$d.out >> test-model-full-fledged.$d
        rm output/train-g$d.mod*
    done
    
#    echo "" > train-model-plus-lm.$d
#	echo "" > test-model-plus-lm.$d
#    for i in $(seq 1 1 $N)
#    do
#        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE true true $ITERATION data/train.en.lm > train-model-plus-lm.$d.out 2> train-model-plus-lm.$d.log
#        tail -n2 train-model-plus-lm.$d.out >> train-model-plus-lm.$d
#        ./test-model.sh $d $BEAM $VERBOSE true true data/train.en.lm > test-model-plus-lm.$d.out 2> test-model-plus-lm.$d.log
#        tail -n1 test-model-plus-lm.$d.out >> test-model-plus-lm.$d
#        rm output/train-g$d.mod*
#    done
done

