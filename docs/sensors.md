# Sensores Modbus reutilizables

El repositorio [galio-sensors](https://github.com/galio-electronics/galio-sensors)
contiene el mapa, validacion, lectura y formato decimal de QDY30A.
No depende de `firmware/common`, `station.h`, eventos ni telemetria.
DHT22 permanece en `firmware/common` con su implementacion Timer1/INT0.

## Nueva app

1. Agregar `simple_master` y `galio-sensors` al manifiesto propio
   `config/dependencies.json`, fijando commits completos y huellas SHA256.
   Las apps `station-qdy30a` y `station-qdy30a-lte` tienen ejemplos actuales.
2. Configurar UART, baudrate, DE/RE, timeout y espera de TX completo en la app.
3. Incluir `simple_master.c` una vez (las demos lo hacen con library_config.h).
4. Definir `GALIO_SENSORS_CCS`, incluir el adaptador
   `galio-sensors/adapters/modbus_simple_master.h` y la implementacion
   `galio-sensors/sensors/qdy30a/qdy30a.c` una vez.
5. Inicializar el bus y llamar `qdy30a_read(direccion, &muestra)` cuando lo decida
   la app. Solo publicar la muestra si retorna GS_OK.

Una app sin common puede incluir simple_master directamente. El driver no
requiere scheduler, diagnosticos, Ethernet, LTE ni gestores de eventos.
La documentacion del repo explica el contrato para sustituir simple_master.

## Configuracion y multiples sensores

El driver define FC03, registros 0..6, tipos con signo y validacion de decimales.
La app define direccion, intervalo y configuracion del bus. Cada llamada recibe
direccion y destino independientes; serializar consultas del mismo bus y
conservar el estado/ultima muestra por sensor en la app. Las consultas siguen
siendo bloqueantes. Este cambio no agrega un planificador ni cambia el timeout.

Se conservan los campos JSON y errores 1..5 de ambas demos. La unidad se entrega
como codigo; sigue pendiente confirmar la unidad fisica del codigo 17.
El driver nunca modifica calibracion, direccion ni baudrate del sensor.

## Validacion de la extraccion

Ambas apps compiladas con CCS PCH 5.081, sin errores ni advertencias.
Las pruebas C del repo de sensores cubren transporte simulado, muestras
independientes, datos invalidos y formato decimal de todo el rango int16.
No se ejecutaron en este equipo: no hay compilador C de escritorio disponible.
La compilacion no sustituye una prueba fisica posterior en ambas apps.
