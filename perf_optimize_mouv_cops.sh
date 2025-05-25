#!/usr/bin/env bash
# Teste ./game contre plusieurs binaires et plusieurs fichiers .txt
# Compte uniquement les victoires de ./game (Cops win! ou Robbers win!)

shopt -s nullglob

start_time=$(date +%s)

############################
# Paramètres faciles à éditer
############################
opponents=(bin/mid bin/high)              # autres binaires
runs=3                          # nombre d’itérations
game=./game_opti_mouv_cops                      # ton programme
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
    -p)
      i=$((i + 4))
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



error=0
current_run=0
total_runs=$((runs * ${#inputs[@]} * ${#opponents[@]} * 2))

for txt in "${inputs[@]}"; do
  for opp in "${opponents[@]}"; do
    for ((i=1; i<=runs; i++)); do

      # ./game joue Robber
      ((current_run++))
      output="$($python_cmd server.py "./$opp" "$game" "$txt" 0 2>&1)"
      res="$(printf '%s' "$output" | tail -n 2 | head -n 1 | tr -d '\r' | xargs)"

      if [[ "${output,,}" == *"disqualified"* || "${output,,}" == *"timeout"* ]]; then
        ((error++))
        [[ $verbose == true ]] && echo "[ERREUR] $python_cmd server.py $opp ./game $txt 0"
      fi

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
      output="$($python_cmd server.py "$game" "./$opp" "$txt" 0 2>&1)"
      res="$(printf '%s' "$output" | tail -n 2 | head -n 1 | tr -d '\r' | xargs)"

      if [[ "${output,,}" == *"disqualified"* || "${output,,}" == *"timeout"* ]]; then
        ((error++))
        [[ $verbose == true ]] && echo "[ERREUR] $python_cmd server.py ./game $opp $txt 0"
      fi

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









total_score=0
count=0

for opp in "${opponents[@]}"; do
  total_cop=0
  total_rob=0

  for f in "${inputs[@]}"; do
    cop=${wins_cop_file_opp["${f}_${opp}"]}
    rob=${wins_robber_file_opp["${f}_${opp}"]}

    total_cop=$((total_cop + cop))
    total_rob=$((total_rob + rob))
  done

  total_opp=$(( runs * ${#inputs[@]} ))
  ptotal=$(awk -v c="$total_cop" -v r="$total_rob" -v t="$total_opp" \
                'BEGIN { printf "%.4f", (c + r) / (2 * t) }')

  total_score=$(awk -v s="$total_score" -v p="$ptotal" \
                      'BEGIN { printf "%.4f", s + p }')
  count=$((count + 1))
done


avg_score=$(awk -v s="$total_score" -v c="$count" \
                  'BEGIN { if (c > 0) printf "%.2f", (s / c) * 100; else print 0 }')


echo "$avg_score"

