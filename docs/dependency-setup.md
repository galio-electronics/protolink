# Dependencias por aplicación

Cada app declara sus repositorios en `config/dependencies.json` y los descarga
en su propia carpeta `galio/`. No se lee un manifiesto raíz ni se comparten
checkouts entre apps: pueden fijar versiones diferentes del mismo driver.

```json
{
  "schema_version": 1,
  "directory": "galio",
  "libraries": {}
}
```

Para agregar un repositorio, insertar una entrada en `libraries`, por ejemplo
`mi_driver`, con `url` (HTTPS GitHub, terminada en `.git`, sin tokens), `commit`
(SHA Git completo de 40 caracteres) y `sha256` (mapa de rutas de archivos a sus
huellas SHA256, normalizando CRLF a LF). Las apps existentes muestran entradas
reales listas para usar. El nombre de la entrada será la carpeta en `galio/`.

Desde la raíz:

```powershell
./tools/run.ps1 deps-sync --profile station-demo
./tools/run.ps1 dependencies --profile station-demo
./tools/run.ps1 build --profile station-demo
```

Para una app copiada usar `--config firmware/apps/mi-app/config/project.json`.
La compilación prepara automáticamente todas las dependencias de esa app.
`--all` se limita también a su manifiesto. Incluir los drivers como
`#include <mi_driver/driver.h>`; el compilador recibe el `galio/` local.
Agregar la implementación una sola vez, según las convenciones del driver.

El gestor verifica origin, commit, archivos y cambios locales. Nunca ejecuta
pull/reset/clean sobre repositorios existentes. Si hay diferencias, detiene la
compilación y conserva los archivos. Las descargas fallidas quedan en carpetas
temporales para diagnóstico. Para cambiar versiones, conservar o mover el checkout
anterior antes de volver a preparar las dependencias.

Si Windows bloquea el renombrado de una descarga verificada (errores 5, 32 o 33),
el gestor reintenta hasta ocho veces, durante unos diez segundos. Si el bloqueo
persiste, conserva la carpeta `.nombre-download-*` y muestra su ruta. Cerrar
los procesos que la esten usando y repetir el mismo comando `deps-sync`.
El gestor vuelve a comprobar origin, commit, limpieza y huellas de las descargas
anteriores; reutiliza una que coincida sin volver a descargar. Carpetas incompletas,
modificadas o de otra version quedan intactas y no se instalan.

Los repositorios privados usan Git Credential Manager: autenticar Git previamente;
no guardar tokens en URLs ni archivos versionados. Un checkout ya verificado se
reutiliza sin conexión. Las antiguas descargas del `galio/` raíz quedan intactas,
pero las herramientas ya no las usan.

Para sensores Modbus, ver [sensores reutilizables](sensors.md): las apps QDY30A
fijan `galio-sensors` para el driver y `simple_master` para el transporte.
