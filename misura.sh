#get number of clients as argument
NCLIENT=$1
unlink supervisor_last.log
unlink client_last.log

echo "Now extracting stats from logs"
awk '/SUPERVISOR ESTIMATE/ {print $3 " " $5 " " $8}' supervisor.log | tail -$NCLIENT | sort -nk1 | uniq > supervisor_last.log
awk '/SECRET/ {print $4 " " $2}' client.log| head -$NCLIENT | sort -nk1 > client_last.log
awk -v ncli="$NCLIENT" '{s+=$3} END {print "Received a total of " s " estimates from " ncli " clients."}' supervisor_last.log

IFS=$'\n'
CORRECT=0
TOT=0
JIT_TOT=0
DELTA=0

for line in $(cat supervisor_last.log); 
    do
        ESTIM=$(cut -d' ' -f1 <<< $line)
        ID=$(cut -d' ' -f2 <<< $line)
        SRVN=$(cut -d' ' -f3 <<< $line)
        CLIENT_LINE=$(grep $ID client_last.log)
        CLIENT_SECRET=$(($(cut -d' ' -f1 <<< $CLIENT_LINE) + 0))
        CLIENT_ID=$(cut -d' ' -f2 <<< $CLIENT_LINE)
        printf "ID: %8s Secret: %4d Server estimate: %4d\n" "$ID" "$CLIENT_SECRET" "$ESTIM"
        ((TOT++))
        DELTA=$(($ESTIM - $CLIENT_SECRET))
        if [ $DELTA -lt 25 ] && [ $DELTA -gt -25 ]; then
            ((CORRECT++))
        fi
        ((JIT_TOT = JIT_TOT + DELTA))
    done
echo "Correctly estimated secrets: $CORRECT out of $TOT"
awk -v jit="$JIT_TOT" -v tot="$TOT" 'BEGIN {print "Mean error: " (jit/tot) "ms"; exit}'