#!/bin/bash

source config.txt

#Recorre la lista de servicios especificados
for servicio in $SERVICIOS
do
    #Obtiene el estado actual del servicio
    estado=$(systemctl is-active "$servicio")

    #Registra el estado del servicio en el log
    echo "$(date) $servicio : $estado" >> "$LOG"

    #Si el servicio no esta activo, intenta reiniciarlo
    if [ "$estado" != "active" ]; then

        systemctl restart "$servicio"

        #Verifica el estado del servicio después del reinicio
        nuevo_estado=$(systemctl is-active "$servicio")

        curl -s -X POST \
        "https://api.telegram.org/bot$TOKEN/sendMessage" \
        -d chat_id="$CHAT_ID" \
        -d text="$servicio estaba inactivo. Estado actual: $nuevo_estado"

    fi
done