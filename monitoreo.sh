#!/bin/bash

source config.txt

#Permite modificar los umbrales mediante argumentos
cpu=${1:-$UMBRAL_CPU}
disco=${2:-$UMBRAL_DISCO}

#Obtiene el porcentaje actual de uso de CPU y disco
uso_cpu=$(top -bn1 | grep "Cpu(s)" | awk '{print 100-$8}')
uso_cpu=${uso_cpu%.*}

uso_disco=$(df / | awk 'NR==2 {print $5}' | sed 's/%//')

#Registra el uso de CPU y disco en el log
echo "$(date) CPU:$uso_cpu Disco:$uso_disco" >> "$LOG"

#Verifica si el uso de CPU o disco supera los umbrales y envía una alerta a Telegram
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