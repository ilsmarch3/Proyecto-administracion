#!/bin/bash

hosts=$1
script=$2

#Verifica que se proporcionen los argumentos necesarios
if [ $# -ne 2 ]; then
    echo "Uso: $0 hosts.txt script.sh"
    exit 1
fi

while read host
do

    echo "Conectando a $host"

    #Copia el script al equipo remoto mediante SCP
    scp "$script" "$host:/tmp/"

    ssh "$host" "chmod +x /tmp/$(basename $script)"
    
    salida=$(ssh "$host" "bash /tmp/$(basename $script)")

    echo "$salida" > "reporte_$host.txt"

done < "$hosts"