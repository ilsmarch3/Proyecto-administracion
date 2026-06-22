#!/bin/bash

source config.txt

fecha=$(date +%Y%m%d_%H%M%S)

archivo="respaldo_$fecha.tar.gz"

tar -czf "$archivo" $DIRECTORIOS

if [ -f "$archivo" ] && [ -s "$archivo" ]; then

    tamano=$(du -h "$archivo" | cut -f1)

    echo "$(date) Respaldo generado: $archivo" >> "$LOG"

    curl -s -X POST \
    "https://api.telegram.org/bot$TOKEN/sendMessage" \
    -d chat_id="$CHAT_ID" \
    -d text="Respaldo creado: $archivo Tamaño: $tamano"
else
    echo "$(date) Error al generar respaldo" >> "$LOG"
fi