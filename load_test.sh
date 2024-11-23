# #!/bin/bash

# # Configuration
SERVER_IP="127.0.0.1"  # Replace with actual server IP
PORT=$1  # Replace with actual port number
MAX_CLIENTS=2
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
#             echo "checkout abc.txt" >> "$file"
#         done
#         echo "quit" >> "$file"

# done


for ((clients=1; clients<=MAX_CLIENTS; clients++)); do
    echo "Running $clients client(s)"

    # Run clients in parallel and capture durations
    for ((i=1; i<=$clients; i++)); do
        
        (
        
        start_time=$(date +%s)
        # Simulate client interactions
        ./client "$SERVER_IP" "$PORT" < "input/inp_1.txt"
        # sleep 1

        end_time=$(date +%s.%N)
        
        total_time=$(echo "$end_time - $start_time - 1" | bc)
        
        echo "$total_time" >> "$temp_file"
        ) &
    done

    wait


    total_time=0
    while read -r line
    do
        total_time=$(echo "$total_time + $line" | bc -l)
    done < "$temp_file"
    echo "total: $total_time"
    
    echo "$clients $total_time" >> "$results_file"
    

done

# python3 plot.py $results_file
echo "Benchmark complete. Results in $results_file"
