MAX_CLIENTS=10
MAX_COMMANDS=10

if [ ! -d "input" ]; then
    mkdir input
fi
for ((clients=1; clients<=MAX_CLIENTS; clients++)); do
        file="input/inp_$clients.txt"
        > "$file"
        username="user_$clients"
        password="pass_$clients"
        echo "username: $username"
        echo "$username" >> "$file"
        echo "$password" >> "$file"
        for ((j=1; j<=MAX_COMMANDS; j++)); do
            echo "checkout file.txt" >> "$file"
        done
        echo "quit" >> "$file"

done