# Protolink: placas, código común y demos

Biblioteca de Galio Electronics para PIC18F67K40 y CCS PCH **5.081**.
El repositorio contiene encabezados de placas, módulos compartidos, bootloaders
y apps de demostración. Cada app se configura y compila por separado.

## Estructura

```text
station.h, v1.h, v2.h, excore.h  Encabezados de las placas
firmware/common/                Código compartido
firmware/bootloader/            Bootloaders
firmware/apps/<app>/            Demos y su workspace
  main.c, app_config.h          Código y opciones de la demo
  config/project.json          Fuente, compilador y puerto de la app
  config/dependencies.json     Repositorios y versiones propios
  galio/<dependencia>/          Descargas de esa app (ignorado por Git)
  build/<perfil>/              Resultados de esa app (ignorado por Git)
firmware/examples/              Ejemplos originales y station-client
tools/                         Herramientas compartidas
```

## Trabajar con una app

Abrir su archivo `.code-workspace` y ejecutar **Compilar** con Ctrl+Shift+B.
Las tareas permiten preparar dependencias, seleccionar COM, cargar y monitorear.
Instalar CCS 5.081 y Python 3; `tools/setup.ps1` prepara pyserial en `.venv`.

Desde la raíz:

```powershell
./tools/run.ps1 build --profile station-demo
./tools/run.ps1 build --config firmware/apps/station-demo/config/project.json
./tools/run.ps1 deps-sync --profile station-rtc-sd
./tools/run.ps1 tests
```

`--profile` encuentra las apps por nombre de carpeta, sin registro central.
Para una copia con otro nombre, usar su workspace o `--config` y actualizar el
nombre del perfil en su `config/project.json`. Las rutas de fuentes son relativas
a la carpeta de la app. Los ajustes opcionales de equipo en `config/local.json`
de la raíz se aplican primero; el `config/local.json` de cada app tiene prioridad.
La selección COM compartida se conserva para comodidad al usar la misma placa.

## Crear una app o un proyecto de cliente

Copiar [station-demo](firmware/apps/station-demo/README.md), cambiar su nombre y
agregar sus dependencias. No hay firmware, manifiesto ni workspace maestro.
Las demos actuales conservan su código y sirven como ejemplos para nuevos sensores.

Para un proyecto privado, partir de una copia o fork del repositorio manteniendo
los encabezados, `firmware/common` y `tools`; conservar las demos que sean útiles.
Configurar las dependencias privadas en la app y autenticar Git en el equipo.
No versionar credenciales, `galio`, `.venv` ni resultados de compilación.

Ver [desarrollo](docs/development.md), [dependencias](docs/dependency-setup.md),
[hardware](docs/hardware.md), [eventos](docs/event-manager.md) y
[validaciones históricas](docs/validation.md).
