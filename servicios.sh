#!/bin/bash

source config.txt

for servicio in $SERVICIOS
do

    estado=$(systemctl is-active "$servicio")

    echo "$(date) $servicio : $estado" >> "$LOG"

    if [ "$estado" != "active" ]; then

        systemctl restart "$servicio"

        nuevo_estado=$(systemctl is-active "$servicio")

        curl -s -X POST \
        "https://api.telegram.org/bot$TOKEN/sendMessage" \
        -d chat_id="$CHAT_ID" \
        -d text="$servicio estaba inactivo. Estado actual: $nuevo_estado"

    fi
done