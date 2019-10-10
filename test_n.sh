#!/bin/env bash
#test with n clients

#cleaning up a little
unlink supervisor.log
unlink client.log
NCLIENT=$1

echo "Launching supervisor"
./supervisor 8  1>>supervisor.log &
sleep 2

echo "Launching $NCLIENT clients"
for i in $(seq $NCLIENT)
    do
        ./client 5 8 20  1>>client.log &
    done

#waiting for the clients to finish
sleep 60

echo "Sending double SIGINT to supervisor"
pkill -INT supervisor
pkill -INT supervisor

sleep 1
echo "Now printing stats"
bash misura.sh $NCLIENT