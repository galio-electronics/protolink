# Preparar dependencias en otra computadora

Las librerias se descargan en `galio/` dentro de Protolink. No es necesario
clonarlas manualmente al lado del proyecto. Cada carpeta conserva su propio
repositorio Git. `galio/` se ignora al versionar Protolink.

## Primera compilacion

Instalar Git para Windows, Python 3 con launcher `py` y CCS PCH 5.081 con su
licencia. Para cargar firmware se necesita tambien el controlador USB CP210x.
Abrir de nuevo VS Code despues de instalar Git para actualizar PATH.

1. Clonar Protolink en cualquier carpeta local.
2. Abrir el workspace general o el dedicado a la aplicacion.
3. Ejecutar Compilar. `run.ps1` prepara el entorno Python si falta; el build
   descarga solo las librerias de esa aplicacion y verifica sus versiones.

Ejemplo desde la raiz de Protolink:

```powershell
.\tools\run.ps1 build --profile station-hw-test
```

La primera descarga requiere red y acceso a los repositorios. Despues, con
Python/CCS preparados y las dependencias presentes, el build no hace clone,
fetch ni pull: puede compilar sin conexion.

Si CCS esta instalado en otra carpeta, ajustar `compiler` y `ccs_root` en
`config/local.json`. El puerto se selecciona con la tarea **Protolink:
Seleccionar puerto COM**. `library_root` ya no se usa: la raiz de librerias
es siempre `<protolink>/galio`, incluso si una configuracion local antigua
todavia contiene esa propiedad.

Las credenciales privadas de cada aplicacion NO son dependencias descargables.
RAK3172 requiere su `credentials.local.h` y LTE su `lte_config.local.h`, conforme
a sus README. No incluir claves, tokens o contrasenas en el manifiesto.

## Manifiesto unico

[`config/dependencies.json`](../config/dependencies.json) registra:

- `libraries`: nombre, URL GitHub HTTPS, commit completo y SHA256 de fuentes.
- `applications`: lista de librerias necesarias por perfil, incluso lista vacia.
- `directory`: `galio`.

El gestor sustituye al anterior `dependencies.lock.json`; no hay dos fuentes
de versiones. Las huellas se calculan normalizando CRLF a LF. Los clones se crean con
`core.autocrlf=true`: CCS 5.081 falla al procesar algunos `#if` de las
librerias RUI3/Modbus con finales LF. El contenido versionado no cambia;
las reglas `.gitattributes` del repositorio siguen teniendo prioridad.

| Libreria | Repositorio | Aplicaciones |
|---|---|---|
| analog_drivers | [artgasca/analog_drivers](https://github.com/artgasca/analog_drivers) | station-hw-test |
| sd-card | [galio-electronics/sd-card](https://github.com/galio-electronics/sd-card) | station-rtc-sd |
| simple_master | [galio-electronics/simple_master](https://github.com/galio-electronics/simple_master) | station-qdy30a, station-qdy30a-lte |
| rui3_at | [artgasca/rui3_at](https://github.com/artgasca/rui3_at) | station-rak3172 |
| oled_i2c | [galio-electronics/oled_i2c](https://github.com/galio-electronics/oled_i2c) | station-rak3172 |
| lcd_i2c | [artgasca/lcd_i2c](https://github.com/artgasca/lcd_i2c) | station-client, station-dht22 |
| lte-module | [galio-electronics/lte-module](https://github.com/galio-electronics/lte-module) | station-qdy30a-lte |

Station master y blink no requieren repositorios adicionales. Los modulos
comunes son parte de Protolink. Los ejemplos historicos sin perfil no se
preparan automaticamente hasta declarar su perfil y sus dependencias.

## Preparacion manual opcional

```powershell
# Descargar/verificar solo lo necesario para una aplicacion
.\tools\run.ps1 deps-sync --profile station-hw-test

# Descargar/verificar todo el catalogo
.\tools\run.ps1 deps-sync --all

# Solo inspeccionar; no descarga ni modifica
.\tools\run.ps1 dependencies --profile station-hw-test
.\tools\run.ps1 dependencies --all
```

Cada workspace incluye **Protolink: Preparar dependencias**. Los workspaces
dedicados muestran solo las librerias de su perfil. Antes de la primera
descarga, VS Code puede mostrar esas carpetas como inexistentes; ejecutar
Preparar dependencias o Compilar las crea. El workspace general muestra todo
el catalogo; `deps-sync --all` permite preparar todas esas carpetas.

Para repos privados, autenticar Git/Git Credential Manager con una cuenta
que tenga acceso antes de compilar. El gestor no solicita credenciales en una
terminal invisible ni guarda tokens. Un error de permisos o conexion detiene
el build con un mensaje; no se acepta un HEX viejo como nueva compilacion.

## Proteccion de trabajo local y actualizaciones

Una dependencia nueva se clona en una carpeta temporal dentro de `galio/`,
se selecciona el commit exacto en detached HEAD y se verifican sus fuentes.
Solo entonces se instala bajo `galio/<nombre>`.

Los repositorios ya presentes nunca reciben pull, reset, clean, stash o
checkout automatico. Si tienen cambios locales, otro origin o un commit
distinto, el build se detiene y conserva todo. Una carpeta existente sin Git
tampoco se reemplaza. No usar enlaces/junctions como destinos de dependencias.

Para adoptar otra version: publicar el commit en el repositorio de la libreria,
actualizar commit y huellas en el manifiesto y validar las aplicaciones que
la usan. Si la copia local esta en otra version, guardar primero el trabajo y
seleccionar el commit deliberadamente con Git, o mover la copia fuera de
`galio/<nombre>` para que el gestor descargue una nueva. La herramienta no
realiza ese cambio sobre un checkout existente.

Las descargas interrumpidas quedan como `.nombre-download-*`, ignoradas por
Git, para poder diagnosticarlas. Al reintentar se usa otra carpeta temporal;
no se borra automaticamente ninguna carpeta. Se pueden retirar manualmente
cuando no haya una descarga en curso y se haya comprobado su contenido.

## Validacion de esta migracion

Se descargaron los siete repositorios desde GitHub en `galio/` y se verificaron
sus commits/huellas. Las pruebas del gestor usan repositorios Git temporales
locales y verifican seleccion por perfil, checkout de una version anterior a
HEAD remoto, reutilizacion sin red, hashes, errores de descarga y conservacion
de modificaciones, origin/version distintos y carpetas preexistentes.
Ejecutar `.\tools\run.ps1 tests` para repetir las pruebas.

Validacion realizada el 2026-11-04: 27 pruebas aprobadas y nueve perfiles
compilados con CCS 5.081 usando `galio/`. RAK conserva su advertencia previa
`ctx` no utilizado. Se preparo ademas una copia limpia sin dependencias ni
configuracion privada: los builds de QDY30A y del demo de hardware descargaron
sus repositorios desde GitHub automaticamente y terminaron sin errores/avisos.
No se cargo firmware durante esta migracion.
