N=$1
D=$2
SHUFFLE=$3

for d in $(seq 0 1 $D)
do
    rm train.$d test.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d 5 0 $SHUFFLE false 2> /dev/null | tail -n2 >> train.$d
        ./test-model.sh $d 10 0 false 2> /dev/null | tail -n1 >> test.$d
    done
        
    rm train-cube-growing.$d test-cube-growing.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d 5 0 $SHUFFLE true 2> /dev/null | tail -n2 >> train-cube-growing.$d
        ./test-model.sh $d 10 0 true 2> /dev/null | tail -n1 >> test-cube-growing.$d
    done
    
    rm train-plus-bigram.$d test-plus-bigram.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d 5 0 $SHUFFLE true data/train.en.lm 2> /dev/null | tail -n2 >> train-plus-bigram.$d
        ./test-model.sh $d 10 0 true data/train.en.lm 2> /dev/null | tail -n1 >> test-plus-bigram.$d
    done
    
    rm train-non-local.$d test-non-local.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model-non-local.sh $d 5 0 $SHUFFLE true 2> /dev/null | tail -n2 >> train-non-local.$d
        ./test-model.sh $d 10 0 true 2> /dev/null | tail -n1 >> test-non-local.$d
    done
    
done
