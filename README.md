# Proyecto-administracion
# Gestor Automatizado de Servicios

Proyecto final de Programación para Administración de Servicios.

# Archivos del proyecto
- config.txt: Archivo de configuración compartido por todos los scripts. Contiene variables reutilizables como el token y Chat ID de Telegram, umbreales de monitoreo, servicios, host y directorios de respaldo
- hosts.txt: Archivo utilizado por remoto.sh que almacena la lista de equipos remotos a los cuales se establecera conexión mediante SSH
- usuarios.sh: Permite crear, eliminar y modificar usuarios del sistema. Ademas, registra las acciones realizadas y envia notificaciones mediante Telegram
- respaldo.sh: Genera respaldos comprimidos de directorios especificos utilizando tar, verificando su creacion y registrando la informacion en el sistema
- monitoreo.sh: MOnitorea el uso de CPU y disco del sistema, si se superan los umbrales configurados, envia alertas mediante Telegram
- prueba_remoto.sh: Script de prueba utilizado por remoto.sh. Este archivo es enviado y ejecutado en los equipos remotos para comprobar el correcto funcionamiento de la conexión SSH
- servicios.sh: Supervisa el estado de servicios criticos del sistema, intentando reiniciarlos automaticamente en caso de fallo
- remoto.sh: Realiza la copia y ejecucion remota de scripts en diferentes host utilizando SSH y SCP, generando reportes individuales
- red.sh: Verifica la conectividad de hosts y el estado de puertos criticos, clasificando cada dispositivo segun su disponibilidad
- inventario.sh: Genera inventarios automaticos del sistema recopilando información del hardware, memoria, almacenamiento y sistema operativo

# Requisitos

- Linux GNU/Linux
- Bash
- curl
- tar
- cron
- ssh/scp
- netcat
- systemd

# Instalación

```bash
git clone https://github.com/ilsmarch3/Proyecto-administracion.git
cd Proyecto-administracion
chmod +x *.sh
```

# Configuración

Editar `config.txt` e ingresar:

```bash
TOKEN="TOKEN_DEL_BOT"
CHAT_ID="CHAT_ID"
```

# Ejecución

```bash
./usuarios.sh
./monitoreo.sh
./inventario.sh
./remoto.sh hosts.txt prueba_remoto.sh
```

# Autores

- Ilse Alibeth Martinez Chimal
- Daniel Fernandez Mejia