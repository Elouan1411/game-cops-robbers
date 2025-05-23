#!/usr/bin/env bash
# Teste ./game contre plusieurs binaires et plusieurs fichiers .txt
# Compte uniquement les victoires de ./game (Cops win! ou Robbers win!)

shopt -s nullglob

############################
# Paramètres faciles à éditer
############################
opponents=(bin/low bin/mid bin/high)              # autres binaires
runs=3                          # nombre d’itérations
game=./game                      # ton programme
python_cmd=python3               # ou python
verbose=false                    # mode verbeux
############################

# Gestion de l'option verbose

args=("$@")  # convertit en tableau pour accéder avec un index

i=0
while [ $i -lt $# ]; do
  arg="${args[$i]}"
  case "$arg" in
    -v|--verbose)
      verbose=true
      ;;
    -n|--number)
      i=$((i + 1))
      runs="${args[$i]}"
      ;;
    *)
      echo "Usage: $0 [-v] [-n number]"
      exit 1
      ;;
  esac
  i=$((i + 1))
done

inputs=( test_file/*.txt )
[ ${#inputs[@]} -eq 0 ] && { echo "Aucun .txt trouvé"; exit 1; }

# Associatifs : victoires de ./game
declare -A wins_cop wins_robber
declare -A wins_cop_file wins_robber_file
declare -A wins_cop_file_opp wins_robber_file_opp

for opp in "${opponents[@]}"; do
  wins_cop["$opp"]=0
  wins_robber["$opp"]=0
done

for f in "${inputs[@]}"; do
  wins_cop_file["$f"]=0
  wins_robber_file["$f"]=0
  for opp in "${opponents[@]}"; do
    wins_cop_file_opp["${f}_${opp}"]=0
    wins_robber_file_opp["${f}_${opp}"]=0
  done
done

echo "Tests : ${#opponents[@]} adversaires × ${#inputs[@]} fichiers × $runs×2 parties …"



total_runs=$(( ${#opponents[@]} * ${#inputs[@]} * 2 * runs ))
current_run=0

for txt in "${inputs[@]}"; do
  for opp in "${opponents[@]}"; do
    for ((i=1; i<=runs; i++)); do
      # ./game joue Robber
      ((current_run++))
      res="$($python_cmd server.py "./$opp" "$game" "$txt" 0 2>&1 | tail -n 2 | head -n 1 | tr -d '\r' | xargs)"
      if [[ "$verbose" == true ]]; then
        if [[ "${res,,}" == *"robbers win!"* ]]; then
          emoji="✅"
        else
          emoji="❌"
        fi
        echo "[map: $txt] $opp VS ./game $emoji (run $current_run/$total_runs)"
      fi
      if [[ "${res,,}" == *"robbers win!"* ]]; then
        ((wins_robber["$opp"]++))
        ((wins_robber_file["$txt"]++))
        ((wins_robber_file_opp["${txt}_${opp}"]++))
      fi

      # ./game joue Cop
      ((current_run++))
      res="$($python_cmd server.py "$game" "./$opp" "$txt" 0 2>&1 | tail -n 2 | head -n 1 | tr -d '\r' | xargs)"
      if [[ "$verbose" == true ]]; then
        if [[ "${res,,}" == *"cops win!"* ]]; then
          emoji="✅"
        else
          emoji="❌"
        fi
        echo "[map: $txt] ./game VS $opp $emoji (run $current_run/$total_runs)"
      fi
      if [[ "${res,,}" == *"cops win!"* ]]; then
        ((wins_cop["$opp"]++))
        ((wins_cop_file["$txt"]++))
        ((wins_cop_file_opp["${txt}_${opp}"]++))
      fi
    done
  done
done



# Calcul des totaux
total_general=$(( 2 * runs * ${#inputs[@]} * ${#opponents[@]} ))

printf "\n===== Résultats par adversaire =====\n"
printf "%-12s | %9s | %11s | %9s\n" "Adversaire" "Cop(%)" "Robber(%)" "Total(%)"
printf -- "-------------|-----------|-------------|-----------\n"
for opp in "${opponents[@]}"; do
  cop=${wins_cop[$opp]}
  rob=${wins_robber[$opp]}
  total_opp=$(( 1 * runs * ${#inputs[@]} ))
  
  pcop=$(awk "BEGIN { printf \"%.1f\", ($cop / $total_opp) * 100 }")
  prob=$(awk "BEGIN { printf \"%.1f\", ($rob / $total_opp) * 100 }")
  ptotal=$(awk "BEGIN { printf \"%.1f\", (($cop + $rob) / (2 * $total_opp)) * 100 }")
  
  printf "%-12s | %7s%% | %9s%% | %7s%%\n" \
         "$opp" "$pcop" "$prob" "$ptotal"
done | column -t -s'|'

printf "\n===== Résultats par fichier =====\n"
printf "%-15s" "Fichier"
for opp in "${opponents[@]}"; do
  printf " | %-11s | %-13s | %-9s" "$opp Cop(%)" "$opp Robber(%)" "$opp Total"
done
printf "\n"

printf -- "%-15s" "---------------"
for opp in "${opponents[@]}"; do
  printf " | %s | %s | %s" "-----------" "-------------" "---------"
done
printf "\n"

for f in "${inputs[@]}"; do
  printf "%-15s" "$f"
  for opp in "${opponents[@]}"; do
    cop=${wins_cop_file_opp["${f}_${opp}"]}
    rob=${wins_robber_file_opp["${f}_${opp}"]}
    
    pcop=$(awk "BEGIN { printf \"%.1f\", ($cop / $runs) * 100 }")
    prob=$(awk "BEGIN { printf \"%.1f\", ($rob / $runs) * 100 }")
    ptotal=$(awk "BEGIN { printf \"%.1f\", ($cop + $rob) / (2 * $runs) * 100 }")
    
    printf " | %7s%%     | %9s%%       | %7s%%" "$pcop" "$prob" "$ptotal"
  done
  printf "\n"
done | column -t -s'|'
