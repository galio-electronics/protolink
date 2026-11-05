# Prueba fisica de eventos y ADS1115

Perfil `station-hw-test`, CCS 5.081. Consola USB UART1 a 115200.
Usa el puerto I2C1 comprobado; direccion ADS1115 0x49 (ADDR a VDD),
AIN0/AIN1 single-ended, PGA +/-6.144 V, 128 SPS, conversion single-shot
con timeout de 100 ms. Valores crudos, sin calibracion ni conversion a mA.
RDY se configura como conversion-ready y se observa RA3 como entrada.
Se reutilizan las definiciones del ADS existente en `../protolink-projects`;
no se usa su bucle de conversion sin timeout.

Comandos: `info`, `status`, `LOG STATUS`, `LOG COUNT`, `LOG READ 0`,
`HEALTH STATUS`, `ADC PROBE` (consulta 0x48..0x4B).
`LOG CLEAR` existe, pero no se usa en las pruebas de retencion.
Los mensajes comunes ahora muestran estados y eventos por nombre, CRC,
parametros, uptime y timestamp, conservando los 32 bytes originales en HEX.

## Evidencia 2026-11-04: placa identificada por el usuario como Monitor

COM33, bootloader Station/Monitor BOOT:1.2. El usuario aclaro despues de
la prueba que esta placa NO tiene ADS1115 montado. Las observaciones ADC
no son evidencia de averia: el perfil de prueba Station habilita un componente
ausente en Monitor. No usar ese perfil como configuracion de produccion Monitor.

- EEPROM: escritura y verificacion reales; cuatro registros iniciales leidos
  con CRC correcto desde PC y PIC; identicos byte por byte despues de MCLR.
- Retencion tambien observada despues de cargar el siguiente firmware.
- Scan completo observado: 5929..5936 ms; main y USB siguieron funcionando.
- MCLR: PCON0=0x37; al salir de actualizacion PCON0=0x33, clasificado UNKNOWN.
- Sin perdidas: tx_drop=0, rx_drop=0, cola perdida=0, errores EEPROM=0.
- RTC respondio, pero reporto oscilador detenido (RTC_STOPPED=3); no se
  ajusto hora ni se borraron indicadores de alimentacion.
- ADC 0x48..0x4B sin lectura valida, esperado por ausencia fisica confirmada.
- No se hicieron cortes de alimentacion ni pruebas de calibracion analogica.

Captura principal: `logs/20261104-210349-hardware-events.log` y JSON asociado.
Prueba repetible: `python tools/hardware_event_probe.py --reset`.
Carga: `tools/run.ps1 build-upload --profile station-hw-test --seconds 8`.
La aplicacion queda ejecutandose al cerrar el monitor USB.

## Evidencia 2026-11-04: Station conectada en COM26

El usuario sustituyo Monitor por Station; COM26 es ahora el puerto guardado.
Se cargo el perfil de prueba y se observaron estos resultados reales:

- ADS1115: solo 0x49 respondio con ACK y lectura de CONFIG correcta.
  0x48/0x4A/0x4B devolvieron NACK en la fase de direccion, como se esperaba.
- AIN0: 37..39 cuentas; AIN1: 17..18 cuentas, en las capturas realizadas.
  Configuraciones C180/D180 verificadas por lectura. Cero errores ADC.
  Esto valida comunicacion y conversion, no exactitud ni calibracion 4-20 mA.
- RDY en RA3: alto observado durante conversion y bajo al completarse en
  las diez conversiones de la captura final (`RDY_ocupado=1 listo=0`).
  En single-shot no se asume que leer el dato libere RDY; se contrasta el
  nivel durante conversion frente al de conversion terminada.
- EEPROM: ocho lecturas con CRC correcto; cuatro registros identicos antes
  y despues de MCLR. Historial conservado tambien entre las cargas de prueba.
  Exploracion observada de 5945..5949 ms; cero errores, corruptos o perdidas.
- USB: tx_drop=0, rx_drop=0. Mensajes descriptivos y registros HEX completos.
- RTC: responde pero devuelve RTC_STOPPED=3. No se modifico su hora.
  Salud global DEGRADADO por RTC; ADS1115 y EEPROM OK. Eventos con uptime
  y TIME_INVALID mientras no haya hora confiable.
- No se provocaron cortes de alimentacion ni se aplicaron corrientes patron.

Capturas Station:
- `logs/20261104-210905-hardware-events.log`: direcciones, salud, CRC y MCLR.
- `logs/20261104-210905-hardware-events.json`: comparacion binaria de historial.
- `logs/20261104-211009-183287-station-hw-test.log`: transiciones RDY finales.

Build final CCS 5.081: ROM 21016 bytes, RAM main 857, RAM maxima 973,
7 niveles de stack; cero errores/avisos. Se dejo ese firmware ejecutando en
Station; el monitor PC finalizo y COM26 quedo libre. No se borro historial.

Referencia de RDY y direccionamiento:
[TI ADS1115, secciones 7.3.8 y 8](https://www.ti.com/lit/ds/symlink/ads1115.pdf).
