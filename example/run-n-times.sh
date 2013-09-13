set -x

N=$1
D=$2
SAMPLES=$3
VERBOSE=$4
SHUFFLE=$5
MAX_SEQ=$6

ITERATION=10
BEAM=100

for d in $(seq 0 1 $D)
do
#	echo "" > train-model.$d
#	echo "" > test-model.$d
#    for i in $(seq 1 1 $N)
#    do
#        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE false false $MAX_SEQ $ITERATION > train-model.$d.out 2> train-model.$d.log
#        tail -n2 train-model.$d.out > train-model.$d
#        ./test-model.sh $d $BEAM $VERBOSE false false $MAX_SEQ > test-model.$d.out 2> test-model.$d.log
#        tail -n1 test-model.$d.out > test-model.$d
#    done
        
	echo "" > train-model-cube-growing.$d
	echo "" > test-model-cube-growing.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE true false $MAX_SEQ $ITERATION > train-model-cube-growing.$d.out 2> train-model-cube-growing.$d.log
        tail -n2 train-model-cube-growing.$d.out > train-model-cube-growing.$d
        ./test-model.sh $d $BEAM $VERBOSE false false $MAX_SEQ > test-model-cube-growing.$d.out 2> test-model-cube-growing.$d.log
        tail -n1 test-model-cube-growing.$d.out > test-model-cube-growing.$d
    done
    
    echo "" > train-model-full-fledged.$d
	echo "" > test-model-full-fledged.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d $SAMPLES $VERBOSE $SHUFFLE true true $MAX_SEQ $ITERATION > train-model-full-fledged.$d.out 2> train-model-full-fledged.$d.log
        tail -n2 train-model-full-fledged.$d.out > train-model-full-fledged.$d
        ./test-model.sh $d $BEAM $VERBOSE false false $MAX_SEQ > test-model-full-fledged.$d.out 2> test-model-full-fledged.$d.log
        tail -n1 test-model-full-fledged.$d.out > test-model-full-fledged.$d
    done
    
done
