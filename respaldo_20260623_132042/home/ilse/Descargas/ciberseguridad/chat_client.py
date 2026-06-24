import socket
import threading
import sys
import platform

def recibir_mensajes(sock):
    while True:
        try:
            data = sock.recv(1024)
            if not data:
                break
            sys.stdout.write('\r' + data.decode('utf-8') + '\nTú: ')
            sys.stdout.flush()
        except:
            print("\n[ERROR] Conexión perdida con el servidor.")
            break

def iniciar_cliente():
    # AQUÍ DEBEN PONER LA MAC DE TU COMPUTADORA (SERVIDOR)
    server_mac = "50:FE:0C:3A:43:85" 
    canal = 4
    
    client_sock = None
    sistema = platform.system()

    print(f"Detectado sistema operativo: {sistema}")
    print(f"Intentando conectar a {server_mac} en el canal {canal}...")

    try:
        if sistema == "Windows":
            # Conexión nativa en Windows
            client_sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
            client_sock.connect((server_mac, canal))
            
        elif sistema == "Linux":
            # Conexión nativa en Linux (Usa AF_BLUETOOTH pero con sintaxis de tupla de dirección de Linux)
            # Nota: Requiere que el Bluetooth esté encendido y desmutear el puerto si es necesario
            client_sock = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
            client_sock.connect((server_mac, canal))
            
        else:
            print("[ERROR] Sistema operativo no soportado de forma nativa para este script.")
            return

        print("¡Conectado con éxito al chat grupal por Bluetooth!")

        # Hilo para recibir mensajes de forma asíncrona
        hilo_receptor = threading.Thread(target=recibir_mensajes, args=(client_sock,))
        hilo_receptor.daemon = True
        hilo_receptor.start()

        # Bucle para enviar mensajes
        while True:
            mensaje = input("Tú: ")
            if mensaje.lower() == 'salir':
                break
            if mensaje.strip():
                client_sock.send(mensaje.encode('utf-8'))

    except Exception as e:
        print(f"[ERROR DE CONEXIÓN] No se pudo conectar al servidor: {e}")
        print("Asegúrate de que los dispositivos estén emparejados en el sistema operativo.")
    finally:
        if client_sock:
            client_sock.close()

if __name__ == "__main__":
    iniciar_cliente()
