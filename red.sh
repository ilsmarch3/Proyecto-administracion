#!/bin/bash

source config.txt

for host in $HOSTS
do

    if ping -c 2 "$host" &>/dev/null; then

        abiertos=0

        for puerto in $PUERTOS
        do

            nc -z "$host" "$puerto" &>/dev/null

            if [ $? -eq 0 ]; then
                abiertos=$((abiertos+1))
            fi
        done

        if [ $abiertos -eq ${#PUERTOS[@]} ]; then
            estado="Accesible"

        elif [ $abiertos -gt 0 ]; then
            estado="Parcialmente accesible"

        else
            estado="Sin puertos abiertos"
        fi

    else
        estado="Sin respuesta"

        curl -s -X POST \
        "https://api.telegram.org/bot$TOKEN/sendMessage" \
        -d chat_id="$CHAT_ID" \
        -d text="Host $host no responde"
    fi

    echo "$(date) $host -> $estado" >> "$LOG"

done