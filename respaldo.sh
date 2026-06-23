#!/bin/bash

source config.txt

fecha=$(date +%Y%m%d_%H%M%S)

archivo="respaldo_$fecha.tar.gz"

#Crea el respaldo comprimido de los directorios especificados
tar -czf "$archivo" $DIRECTORIOS

#Verifica que el archivo se haya creado correctamente y no esté vacío
if [ -f "$archivo" ] && [ -s "$archivo" ]; then

    tamano=$(du -h "$archivo" | cut -f1)

    #Registra el evento en el log y envía un mensaje a Telegram
    echo "$(date) Respaldo generado: $archivo" >> "$LOG"
    curl -s -X POST \
    "https://api.telegram.org/bot$TOKEN/sendMessage" \
    -d chat_id="$CHAT_ID" \
    -d text="Respaldo creado: $archivo Tamaño: $tamano"
else
    #Registra el error en el log si el respaldo no se creó 
    echo "$(date) Error al generar respaldo" >> "$LOG"
fi