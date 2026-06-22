# Proyecto-administracion
# Gestor Automatizado de Servicios

Proyecto final de Programación para Administración de Servicios.

# Scripts incluidos

- usuarios.sh
    Permite crear, eliminar y modificar usuarios del sistema. Ademas, registra las acciones realizadas y envia notificaciones mediante Telegram
- respaldo.sh
- monitoreo.sh
- servicios.sh
- remoto.sh`
- red.sh`
- inventario.sh

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