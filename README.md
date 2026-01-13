# 🚀 Protolink Projects – Firmware & Tools Platform

Este repositorio contiene el ecosistema completo de desarrollo para la plataforma **Protolink** de **Galio Electronics**, incluyendo firmware, librerías, ejemplos, proyectos de producción y herramientas de PC.

---

## 📁 Estructura del repositorio
```
protolink-projects/
│
├─ firmware/
│ ├─ protolink-v2/
│ │ ├─ examples/ # Ejemplos listos para usar
│ │ ├─ production/ # Proyectos reales de clientes / planta
│ │ └─ common/ # Configuración de hardware compartida
│ │
│ └─ protolink-v1/ # (Opcional) soporte legado
│
├─ libs/
│ ├─ modbus/ # subtree → https://github.com/artgasca/modbus

│ └─ analog_drivers/ # subtree → https://github.com/artgasca/analog_drivers

│
├─ pc-tools/
│ ├─ GalioLoad/ # Loader + Terminal Serial
│ └─ otros_tools/
│
├─ docs/
│ ├─ img/
│ └─ notas_tecnicas.md
│
└─ README.md

```
---

## 🧱 Objetivo del repositorio

- Centralizar todos los proyectos Protolink  
- Incluir firmware de demo y producción  
- Integrar librerías externas mediante **git subtree**  
- Permitir desarrollo modular y reutilizable  
- Mantener herramientas de PC (como **GalioLoad**)  

---

## 📦 Librerías integradas (git subtree)

| Carpeta local                 | Repo original                                                 |
|-------------------------------|---------------------------------------------------------------|
| `libs/modbus/`               | https://github.com/artgasca/modbus                           |
| `libs/analog_drivers/`       | https://github.com/artgasca/analog_drivers                   |

---

## 🔧 Comandos (subtree)

### ➕ Agregar librerías (ya configurado)

Desde la raíz de protolink-projects:

```bash
git remote add lib-modbus-ccs https://github.com/tu-org/lib-modbus-ccs.git
git remote add lib-mqtt-serial-ccs https://github.com/tu-org/lib-mqtt-serial-ccs.git
git remote add analog_drivers https://github.com/artgasca/analog_drivers.git
# Agrega los que necesites
```
Verifica:
```bash
git remote -v
```

Agregar cada librería con git subtree
Modbus:
```bash
git subtree add --prefix=libs/lib-modbus-ccs lib-modbus-ccs main --squash
```
MQTT Serial:
```bash
git subtree add --prefix=libs/lib-mqtt-serial-ccs lib-mqtt-serial-ccs main --squash
```
INA226:
```bash
git subtree add --prefix=libs/analog_drivers analog_drivers main --squash
```
Cada comando te deja el código de esa lib como carpeta normal, pero con el historial embebido.
Commit si hace falta (normalmente git ya lo hace con el --squash).


## 🔁 Actualizar librerías
```bash
git subtree pull --prefix=libs/modbus modbus main --squash
git subtree pull --prefix=libs/analog_drivers analog_drivers main --squash
```

## ⬆️ Enviar cambios hacia las librerías
```bash
git subtree push --prefix=libs/modbus modbus main
git subtree push --prefix=libs/analog_drivers analog_drivers main
```

## 📡 Firmware

Organizado por versión de hardware.
Cada proyecto incluye:

src/ – Código fuente

config/ – Pines, defines y opciones del proyecto

docs/ – Información para instalación y soporte

Librerías accesibles directamente en libs/

Ejemplo de include:
```bash
#include "../../../libs/modbus/modbus.h"
#include "../../../libs/analog_drivers/ina226.h"
#include "../config/project_config.h"
```



## 🖥 Herramientas de PC
GalioLoad

Ubicación: pc-tools/GalioLoad/

Funciones:

Entrar a bootloader (DTR/RTS)

Carga de .hex con ACK por línea

Barra de progreso + porcentaje

Terminal serial integrada

UI minimalista

## 🛠 Requisitos de desarrollo
Firmware

CCS Compiler 5.x+

PIC18F67K40 (Protolink V2)

Reloj recomendado: 64 MHz

UART / RS232 / RS485

PC Tools

.NET Framework 4.7.2

Windows 10/11

## 🔄 Flujo recomendado

Crear un nuevo proyecto en examples/ o production/

Incluir librerías desde libs/

Configurar pines y parámetros

Compilar con CCS

Cargar con GalioLoad

Probar en banco o campo

Sincronizar cambios en librerías si aplica:
```bash
git subtree push --prefix=libs/modbus modbus main
```
🏢 Autor

Galio Electronics – Plataforma Protolink
Soluciones industriales, IoT y desarrollo avanzado de hardware + firmware.

🌐 https://galio.dev

📧 desarrollo@galio.dev
