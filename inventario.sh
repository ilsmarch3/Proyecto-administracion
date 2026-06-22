#!/bin/bash

source config.txt

fecha=$(date +%Y%m%d)

archivo="/var/log/inventario_$fecha.txt"

echo "===== INVENTARIO =====" > "$archivo"

echo "Hostname: $(hostname)" >> "$archivo"
echo "Sistema Operativo: $(lsb_release -d | cut -f2)" >> "$archivo"
echo "Kernel: $(uname -r)" >> "$archivo"

echo "" >> "$archivo"

echo "CPU:" >> "$archivo"
lscpu | grep "Model name" >> "$archivo"
lscpu | grep "^CPU(s)" >> "$archivo"

echo "" >> "$archivo"

echo "Memoria RAM:" >> "$archivo"
free -h >> "$archivo"

echo "" >> "$archivo"

echo "Discos:" >> "$archivo"
df -h >> "$archivo"

resumen="Inventario generado en $(hostname)"

curl -s -X POST \
"https://api.telegram.org/bot$TOKEN/sendMessage" \
-d chat_id="$CHAT_ID" \
-d text="$resumen"

echo "Inventario generado correctamente"