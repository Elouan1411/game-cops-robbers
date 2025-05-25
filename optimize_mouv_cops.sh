#!/bin/bash

# Bornes des constantes à faire fluctuer
MIN_A=0
MAX_A=10
MIN_B=0
MAX_B=10

if [ -z "$1" ]; then
  echo "Usage: $0 <nombre_d_essais>"
  exit 1
fi

N_TRIALS=$1
LOGFILE="opti_mouv_cops.log"
PARAMS_FILE="params_mouv_cops.txt"
> "$LOGFILE"  # vide le fichier

best_score=-1
best_combination=""

for ((i = 1; i <= N_TRIALS; i++)); do
    A=$((RANDOM % (MAX_A - MIN_A + 1) + MIN_A))
    B=$((RANDOM % (MAX_B - MIN_B + 1) + MIN_B))


  echo "$A $B" > "$PARAMS_FILE"
  CMD="./perf_optimize_mouv_cops.sh -n 1"
  OUTPUT=$(eval $CMD 2>/dev/null)

  SCORE=$(echo "$OUTPUT" | tr ',' '.' | grep -Eo '[0-9]+\.[0-9]+' | head -n 1)
  if [[ -z "$SCORE" ]]; then
    SCORE=0
  fi

  echo "($i/$N_TRIALS) $A $B -> $SCORE" >> "$LOGFILE"
  echo "Run $i/$N_TRIALS done with $A $B -> $SCORE, remaining $((N_TRIALS - i))"

  if (( $(echo "$SCORE > $best_score" | bc -l) )); then
    best_score=$SCORE
    best_combination="$A $B"
  fi

done

echo "Best score: $best_score with parameters: $best_combination"
