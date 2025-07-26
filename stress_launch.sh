#!/bin/bash 


CLIENT_BIN=/home/userLinux/workspace/pgw/cmake/build/client/pgw_client
IMSI_GEN=/home/userLinux/workspace/pgw/cmake/build/imsi_generator/imsi_generator

NUM_USERS=$1
TOTAL_RPS=$2

if ! [[ -x "$CLIENT_BIN" ]]
then
  echo "Error: udp_client not found or not executable at $CLIENT_BIN"
  exit 1
fi

if ! [[ -x "$IMSI_GEN" ]]
then
  echo "Error: imsi_generator not found or not executable at $IMSI_GEN"
  exit 1
fi

INTERVAL_MS=$(( (1000 * NUM_USERS) / TOTAL_RPS ))
if (( INTERVAL_MS == 0 )); then
  echo "Error: RPS per user too low. Try increasing RPS or reducing user count."
  exit 1
fi

echo "[INFO] Launching $NUM_USERS clients with $RPS_PER_USER rps each"


for ((i=0; i<NUM_USERS; i++));do
    IMSI=$($IMSI_GEN)
    echo "[$i] IMSI: $IMSI"

    $CLIENT_BIN "$IMSI" "$INTERVAL_MS" &
done


echo "All clients launched. Press Ctrl+C to stop."

wait