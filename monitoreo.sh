#!/bin/bash

source config.txt

cpu=${1:-$UMBRAL_CPU}
disco=${2:-$UMBRAL_DISCO}

uso_cpu=$(top -bn1 | grep "Cpu(s)" | awk '{print 100-$8}')
uso_cpu=${uso_cpu%.*}

uso_disco=$(df / | awk 'NR==2 {print $5}' | sed 's/%//')

echo "$(date) CPU:$uso_cpu Disco:$uso_disco" >> "$LOG"

if [ "$uso_cpu" -gt "$cpu" ]; then

    curl -s -X POST \
    "https://api.telegram.org/bot$TOKEN/sendMessage" \
    -d chat_id="$CHAT_ID" \
    -d text="Alerta: CPU al $uso_cpu%"
fi

if [ "$uso_disco" -gt "$disco" ]; then

    curl -s -X POST \
    "https://api.telegram.org/bot$TOKEN/sendMessage" \
    -d chat_id="$CHAT_ID" \
    -d text="Alerta: Disco al $uso_disco%"
fi