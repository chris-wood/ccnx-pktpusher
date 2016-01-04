#!/bin/bash
WINDOWS=( 1 5 10 50 100 )
IP=$1
PORT=$2
FILE=$3

for W in "${WINDOWS[@]}"
do
    ./pusher -w ${W} ${IP} ${PORT} ${FILE} > ${FILE}_${W}.out
    sleep 0.5
done
