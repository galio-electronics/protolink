# Desarrollo de aplicaciones Station

## Firmware maestro

El archivo principal es `firmware/station/main.c`. `app_config.h` define nombre,
versión, diagnóstico y período de estado. El encabezado de placa es `station.h`.
La secuencia de inclusión es configuración > placa > interfaces > librerías >
hooks específicos > `station_app.c`. Es una unidad de compilación CCS; no compilar
los `.c` incluidos como unidades adicionales.

El ejecutor común inicializa GPIO, Timer2, diagnóstico e interrupciones; llama:

- `app_init()` una vez, con GPIO y consola listos.
- `app_task_10ms(inputs_raw)` para adquisición/reglas breves.
- `app_task_100ms()` para procesos periódicos.
- `app_task_1000ms()` para supervisión.

Agregar módulos por función conforme se necesiten. No colocar funciones de clientes
en `station.h`, ni crear copias del pinout. No usar esperas activas o retardos en
las tareas; publicar estado y continuar en la siguiente vuelta. No hay RTOS.
Los intervalos son cooperativos: una tarea larga retrasa a las demás.

## Proyecto cliente de ejemplo

Abrir `firmware/examples/station-client/station-client.code-workspace`.
Este muestra la aplicación, módulos comunes y librerías externas. `Ctrl+Shift+B`
ejecuta **Cliente: Compilar**; **Cliente: Compilar, subir y monitorear** usa siempre
`firmware/examples/station-client/main.c`. El workspace principal puede seguir
seleccionando `station-master`; no afecta a las tareas Cliente.

La plantilla implementa `client_rules.h/.c`: detecta cambios de entradas crudas
y emite un evento. No controla actuadores. Sus funciones son un ejemplo de cómo
agregar una librería local y llamarla periódicamente.

Para otra aplicación, copiar esa carpeta a `firmware/examples/nombre-cliente/`
(o a `firmware/customers/nombre-cliente/`, conservando la misma profundidad),
cambiar `APP_NAME`/`FW_VERSION`, registrar el nuevo perfil en `config/project.json`
y en el selector `.vscode/tasks.json`. Si se copia el workspace cliente, cambiar
su `--profile` fijo para que compile el nuevo programa; no dejar `station-client`.
Se genera `build/<nombre-perfil>/main.hex`, separado de las otras aplicaciones.

La tendencia es consolidar características comunes en el maestro y mantener
configuraciones explícitas por aplicación, no ramas con copias completas del firmware.

## Nueva librería propia

Crear `nombre.h` con guarda, configuración y prototipos, y `nombre.c` con
implementación. Para librerías nuevas el `.h` no incluye el `.c`; la aplicación
incluye el `.c` una vez. No cambiar esta convención en librerías antiguas sin
migrar sus consumidores. Incluir un README y ejemplo mínimo.

Documentar UART/stream, ISR, timer, bus, buffers, tiempos límite, bloqueos y orden
de inicialización. Preferir `init`, `task` y consultas de estado, buffers fijos y
sin asignación dinámica. No configurar reloj/fuses/bootloader dentro del driver.
Las funciones de interrupción solo capturan datos o atienden hardware.

## Agregar una librería existente

1. Revisar `docs/dependencies.md` y su README, configuración e ISR.
2. Declarar macros antes de incluirla; no aceptar defaults de UART1.
3. Configurar PPS/TRIS/bus para el cableado confirmado. `station.h` usa fast_io:
   llamar `output_high` no convierte por sí solo el pin en salida.
4. Incluirla una sola vez según su convención actual.
5. Inicializar desde `app_init` y atenderla desde una tarea sin bloquear.
6. Compilar, revisar RAM, cargar y observar logs con el dispositivo conectado.

`firmware/common/library_config.h` contiene puntos de inclusión optativos mediante
`APP_USE_SIMPLE_MASTER`, `APP_USE_RUI3`, `APP_USE_OLED`, `APP_USE_LCD`.
No están habilitados por defecto y no equivalen a drivers de placa inicializados.
No basta con definir el flag: revisar recursos, tiempos y polaridades pendientes.

## Logs y depuración

Con **Compilar, subir y monitorear**, el monitor recibe el arranque sin cerrar COM.
Los comandos se escriben en la terminal y se envían al pulsar Enter. La consola es
de consulta: no ofrece comandos para activar salidas. Si la terminal no permite
entrada interactiva, usar `monitor --seconds 10 --command info`.

Para analizar una falla, capturar `logs/*.log` y conservar `build/<perfil>/build.json`,
`compile.log` y el listado `.lst`. El manifiesto registra perfil, fuente, SHA256 del
HEX y diferencias de dependencias. Con estos archivos el asistente puede correlacionar
los eventos con el código y repetir capturas acotadas. Los logs no son breakpoints;
la depuración paso a paso sigue perteneciendo a MPLAB/ICSP.

## Herramientas

Las tareas usan `tools/run.ps1` y `.venv`. `config/project.json` selecciona la
aplicación. `--profile` la cambia para una ejecución; `select --profile` la hace
persistente. `config/local.json` sobrescribe rutas y COM de la máquina sin versionarlos.
El build siempre recompila la unidad principal para recoger cambios en todos los
encabezados. Los drivers CCS provienen de la instalación 5.081; no se copian al repo.
