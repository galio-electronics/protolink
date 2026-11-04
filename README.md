# Protolink Firmware — desarrollo en VS Code

Firmware de Galio Electronics, PIC18F67K40, CCS PCH **5.081**.
Station utiliza cristal de 16 MHz, reloj de 64 MHz y bootloader por UART1.

## Inicio diario

1. Abrir **`protolink.code-workspace`** en VS Code (Archivo > Abrir área de trabajo desde archivo).
2. Editar **`firmware/station/main.c`** y `firmware/station/app_config.h`.
3. Guardar los cambios. `Ctrl+Shift+B` ejecuta **Station: Compilar**.
4. Terminal > Ejecutar tarea > **Station: Compilar, subir y monitorear**.
5. Leer mensajes en esa terminal. Escribir `info`, `status`, `inputs` o `errors` y Enter.
6. Usar **Ctrl+C** en el monitor antes de volver a cargar. El monitor y el cargador no pueden abrir COM26 simultáneamente.

El build imprime `PERFIL`, `FUENTE` y `HEX`. **La pestaña abierta no selecciona el programa.**
El perfil predeterminado es `station-master`; compila únicamente `firmware/station/main.c`
como unidad principal CCS, incluyendo los módulos que esta referencia.

## Elegir otra aplicación

Terminal > Ejecutar tarea > **Station: Seleccionar aplicacion**. La selección se
conserva en `config/project.json`, propiedad `active_profile`.

| Perfil | Fuente principal | Diagnóstico |
|---|---|---|
| `station-master` (predeterminado) | `firmware/station/main.c` | UART1, estado cada segundo |
| `station-client` | `firmware/examples/station-client/main.c` | UART1 + eventos de entradas |
| `station-blink` | `firmware/examples/protolink-hello-world.c` | Sin mensajes, solo ejemplo mínimo |

**Station: Ver perfil y fuentes** muestra la selección actual. Para agregar una
aplicación, registrar su ruta en `profiles` y agregar su nombre al selector de
`.vscode/tasks.json`. No se compilan todos los ejemplos del repositorio juntos.

La plantilla tiene un workspace propio:
**`firmware/examples/station-client/station-client.code-workspace`**.
Sus tareas **Cliente:** fijan `--profile station-client`, independientemente del
perfil activo del workspace principal. Editar `client_rules.c` para funciones de
cliente. Ver [guía de desarrollo](docs/development.md).

## Organización

```text
station.h, v1.h, v2.h, excore.h  Encabezados de placa en sus rutas originales
firmware/station/               Aplicación maestra y configuración
firmware/common/                Scheduler común, consola y adaptador de librerías
firmware/examples/              Ejemplos y plantilla cliente
firmware/bootloader/            Fuentes de bootloader, sin herramientas PC
config/                        Perfiles y huellas de dependencias
tools/                         Compilación, auto-boot y monitor Python
tests/                         Pruebas del cargador y flujo
.vscode/                       Tareas y navegación C
docs/                          Hardware, dependencias y desarrollo
build/<perfil>/                HEX, listado, mapa, compile.log y build.json (generados)
logs/                          Capturas serie con fecha y hora (generadas)
.venv/                         Entorno Python local (generado)
```

Las librerías propias permanecen en `../simple_master`, `../rui3_at`,
`../lcd_i2c` y `../oled_i2c`, visibles en el workspace. Los drivers CCS se usan
 desde la instalación original. [Integración y recursos](docs/dependencies.md).

## Equipo y configuración

### Cambiar puerto COM desde cualquier workspace

1. **Terminal > Ejecutar tarea > Protolink: Seleccionar puerto COM**.
2. En la terminal aparece una lista como `1. COM26 - Silicon Labs CP210x`.
3. Escribir el numero o `COM26` y pulsar Enter. `R` actualiza la lista;
   Enter sin texto cancela sin cambiar nada.

La seleccion se guarda automaticamente en `config/local.json` (fuera de Git)
y se comparte entre todos los workspaces/apps de este repositorio. No hace
falta buscar ni editar archivos. La tarea solo enumera puertos: no los abre
ni reinicia la placa. Cerrar el monitor anterior antes de cargar o monitorear.
Los procesos ya abiertos conservan su puerto hasta que se cierren.

Cada carga/monitor muestra la aplicacion, el COM y la descripcion. Si el COM
desaparece, se muestran los disponibles y el nombre de la tarea para elegir otro;
no se cambia de dispositivo automaticamente.

Tambien disponible desde terminal: `.\tools\run.ps1 select-port` o
`.\tools\run.ps1 select-port --port COM26` (requiere puerto detectado).
`--port` en carga/monitor sigue siendo un cambio temporal para esa ejecucion.

Este equipo ya está preparado: COM26, 115200 8N1, CCS 5.081 y pyserial 3.5.
La secuencia de transistores cruzados está verificada en placa.
Para otro equipo, instalar Python 3 con launcher `py` y pip, CCS licenciado y
los drivers del CP210x. Ejecutar `tools/setup.ps1`; también se ejecuta automáticamente
si una tarea no encuentra `.venv`. Requiere acceso a pyserial en pip o su caché.

Editar `config/project.json` para rutas y puerto, o crear `config/local.json`
(no versionado) con `compiler`, `ccs_root`, `library_root` y/o `port` para ajustes
locales. Las rutas de navegación/workspace deben actualizarse si se mueven librerías.

El build de VS Code invoca CCS directamente con los argumentos verificados de
MPLAB y no depende de Makefiles generados. El proyecto MPLAB externo se conserva
para ICSP/uso anterior: **continúa apuntando al ejemplo hello-world**. Su selección
no cambia al seleccionar perfiles en VS Code.

## Tareas y consola

Además de compilar/cargar: **Monitor serie**, **Capturar 30 segundos**, **Validar HEX
sin conectar**, **Verificar dependencias** y **Pruebas de herramientas**.
Los errores CCS se muestran en Problemas con enlaces a archivo/línea. El analizador
C del editor solo ayuda a navegar: se desactivan sus falsos errores de dialecto CCS.

```powershell
# Desde la raíz; estos comandos no dependen de la pestaña activa.
.\tools\run.ps1 build
.\tools\run.ps1 build-upload-monitor
.\tools\run.ps1 build --profile station-client
.\tools\run.ps1 monitor --seconds 30 --command status
.\tools\run.ps1 tests
```

No abrir Galio Flash Tool ni otro monitor durante estas tareas. La carga mantiene
el mismo puerto abierto hasta el monitor para capturar el primer mensaje de arranque.
Ctrl+C termina la captura; si se interrumpe durante escritura, repetir carga completa.

## Hardware y validación

El firmware maestro 0.1.1 mantiene DO1–8 inactivas en alto, lee entradas cada 10 ms
y cambia LED1 cada 500 ms. UART1: RC6/RC7; BOOT: RA4 activo bajo.
Timer2 aporta el tick de 1 ms. [Contrato de hardware](docs/hardware.md).

El loader preserva `0x0000..0x05FF`: aplicación en `0x0600`, ISR en `0x0608`.
El cargador rechaza direcciones fuera de aplicación y excluye fuses del HEX.
Valida checksum, espera ACK/XON por registro y confirmación final. No dispone de
lectura de flash ni rollback; los mensajes de aplicación comprueban el arranque.
No borra restos de una aplicación anterior más allá del final de la nueva.

Documentación adicional: [protocolo serie](docs/serial-protocol.md),
[auto-boot](docs/autoboot.md), [documentación original V2](docs/legacy-v2.md).

Resultados de compilación y pruebas físicas: [validación](docs/validation.md).

El perfil `station-client` incluye ahora el ejemplo LCD 16x2 validado en placa.
El cargador informa líneas/registros y tiempos; la prueba del mismo HEX pasó de
5.215 s a 3.518 s de entrada+carga. Detalles en [validación](docs/validation.md).

Aplicacion DHT22 + LCD + Ethernet UART3: [workspace e instrucciones](firmware/apps/station-dht22/README.md).

Demo RAK3172 UART5 + OTAA ChirpStack US915 canales 0-7:
[workspace, alta y codec](firmware/apps/station-rak3172/README.md).

Demo Qidian QDY30A por RS485 + JSON Ethernet para ThingsBoard:
[workspace e instrucciones](firmware/apps/station-qdy30a/README.md).
## Demo RTC y microSD

Disponible el perfil `station-rtc-sd`: [guía de prueba](firmware/apps/station-rtc-sd/README.md).
Sus drivers SD/FAT32 están en el repositorio hermano `../sd-card`.
