
#!/bin/sh

bin="./low"
txt="input.txt"

# Capture de toute la sortie, stdout + stderr
output=$(python3 server.py "$bin" ./game "$txt" 0 2>&1)

# Affiche toute la sortie
echo "=== SORTIE COMPLETE ==="
printf "%s\n" "$output"

# Récupère la dernière ligne, nettoyage des espaces + caractères \r éventuels
last_line=$(printf "%s" "$output" | tail -n 1 | tr -d '\r' | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')

# Affiche la dernière ligne entre crochets pour voir exactement ce qu'elle contient
echo "=== DERNIÈRE LIGNE ==="
echo "[$last_line]"

# Vérification des cas possibles
if [ "$last_line" = "Cops win!" ]; then
    echo "→ Victoire des gendarmes"
elif [ "$last_line" = "Robbers win!" ]; then
    echo "→ Victoire des voleurs"
else
    echo "→ Résultat inconnu"
fi
