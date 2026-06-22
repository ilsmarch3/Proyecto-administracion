#!/bin/bash

source config.txt

# Función para enviar mensajes a Telegram
enviar_telegram() {
    mensaje=$1

    curl -s -X POST \
    "https://api.telegram.org/bot$TOKEN/sendMessage" \
    -d chat_id="$CHAT_ID" \
    -d text="$mensaje" > /dev/null
}

registrar_log() {
    echo "$(date '+%F %T') - $1" >> "$LOG"
}

while true
do
    echo "===== MENU USUARIOS ====="
    echo "1. Crear usuario"
    echo "2. Eliminar usuario"
    echo "3. Modificar contraseña"
    echo "4. Salir"

    read -p "Seleccione una opcion: " opcion

    case $opcion in

        1)
            read -p "Nombre del usuario: " usuario

            if id "$usuario" &>/dev/null; then
                echo "El usuario ya existe"
            else
                sudo useradd "$usuario"
                echo "Usuario creado"

                registrar_log "Usuario $usuario creado"
                enviar_telegram "Se creó el usuario $usuario"
            fi
        ;;

        2)
            read -p "Usuario a eliminar: " usuario

            if id "$usuario" &>/dev/null; then
                sudo userdel "$usuario"

                registrar_log "Usuario $usuario eliminado"
                enviar_telegram "Se eliminó el usuario $usuario"
            else
                echo "El usuario no existe"
            fi
        ;;

        3)
            read -p "Usuario a modificar: " usuario

            if id "$usuario" &>/dev/null; then
                sudo passwd "$usuario"

                registrar_log "Contraseña modificada para $usuario"
                enviar_telegram "Se modificó la contraseña de $usuario"
            else
                echo "Usuario no encontrado"
            fi
        ;;

        4)
            exit 0
        ;;

        *)
            echo "Opción inválida"
        ;;
    esac
done
