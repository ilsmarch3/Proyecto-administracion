#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Uso: $0 hosts.txt script.sh"
    exit 1
fi

hosts=$1
script=$2

while read host
do

    echo "Conectando a $host"

    scp "$script" "$host:/tmp/" &>/dev/null

    salida=$(ssh "$host" "bash /tmp/$(basename $script)")

    echo "$salida" > "reporte_$host.txt"

done < "$hosts"