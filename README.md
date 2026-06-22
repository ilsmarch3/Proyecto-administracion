# Proyecto-administracion
# Gestor Automatizado de Servicios

Proyecto final de Programación para Administración de Servicios.

# Scripts incluidos

- usuarios.sh: Permite crear, eliminar y modificar usuarios del sistema. Ademas, registra las acciones realizadas y envia notificaciones mediante Telegram
- respaldo.sh: Genera respaldos comprimidos de directorios especificos utilizando tar, verificando su creacion y registrando la informacion en el sistema
- monitoreo.sh: MOnitorea el uso de CPU y disco del sistema, si se superan los umbrales configurados, envia alertas mediante Telegram
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

# Instalación

```bash
git clone <repositorio>
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
```

# Autores

- Ilse Alibeth Martinez Chimal
- Daniel Fernandez Mejia