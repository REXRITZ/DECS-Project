#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: ./load_test.sh <server_port>"
    exit 1
fi

# # Configuration
SERVER_IP="127.0.0.1"  # Replace with actual server IP
PORT=$1  # Replace with actual port number
MAX_CLIENTS=10
MAX_COMMANDS=10


# # Results file
results_file="results.txt"
> "$results_file"

# Temporary file to store durations
temp_file="durations.txt"
> "$temp_file"

if [ ! -d "input" ]; then
    mkdir input
fi
# for ((clients=1; clients<=MAX_CLIENTS; clients++)); do
#         file="input/inp_$clients.txt"
#         > "$file"
#         username="user_$clients"
#         password="pass_$clients"
#         echo "username: $username"
#         echo "$username" >> "$file"
#         echo "$password" >> "$file"
#         for ((j=1; j<=MAX_COMMANDS; j++)); do
#             echo "listall" >> "$file"
#         done
#         echo "quit" >> "$file"

# done
# exit 1

for ((clients=1; clients<=MAX_CLIENTS; clients++)); do
    start_time=$(date +%s)
    # Run clients in parallel and capture durations
    for ((i=1; i<=$clients; i++)); do
        
        (
        
        
        # Simulate client interactions
        ./client "$SERVER_IP" "$PORT" < "input/inp_$i.txt"
        # sleep 1

        ) &
    done

    wait

    end_time=$(date +%s.%N)    
    total_time=$(echo "$end_time - $start_time" | bc)
    # echo "$total_time" >> "$temp_file"
    echo "$clients $total_time" >> "$results_file"


    # total_time=0
    # while read -r line
    # do
    #     total_time=$(echo "$total_time + $line" | bc -l)
    # done < "$temp_file"
    # echo "total: $total_time"
    
    # echo "$clients $total_time" >> "$results_file"
    

done

# python3 plot.py $results_file
echo "Benchmark complete. Results in $results_file"
