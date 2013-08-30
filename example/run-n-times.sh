N=$1

for d in $(seq 0 1 7)
do
    rm train.$d test.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model.sh $d 2> /dev/null | tail -n1 >> train.$d
        ./test-model.sh $d 2> /dev/null | tail -n1 >> test.$d
    done
    
    rm train-non-local.$d test-non-local.$d
    for i in $(seq 1 1 $N)
    do
        ./train-model-non-local.sh $d 2> /dev/null | tail -n1 >> train-non-local.$d
        ./test-model.sh $d 2> /dev/null | tail -n1 >> test-non-local.$d
    done
done
