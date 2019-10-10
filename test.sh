#!/bin/env bash

#cleaning up a little
unlink supervisor.log
unlink client.log

echo "Launching supervisor"
./supervisor 8  1>>supervisor.log &
sleep 2

echo "Launching 20 clients"
for x in $(seq 10);
    do 
        ./client 5 8 20  1>>client.log &
        ./client 5 8 20  1>>client.log &
        sleep 1
    done

for i in $(seq 6);
    do 
        pkill -INT supervisor
        sleep 10
    done

echo "Sending double SIGINT to supervisor"
pkill -INT supervisor
pkill -INT supervisor

sleep 1
echo "Now printing stats"
bash misura.sh 20

