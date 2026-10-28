# Estado y siguiente paso

Todavía no podemos usar Quest 2 como sustituto de PS VR2 en juegos de PS5.
La vía investigada sigue siendo posible, pero no está demostrada ni descartada.

## Qué se hizo

- Recuperé el trabajo anterior de la conversación «Find PSVR2 interception method».
- Contrasté fuentes públicas con versiones fijas. Encontré datos que todavía se contradicen.
- Creé y probé un lector de estructuras guardadas. Ayuda a estudiar cómo se describen las imágenes de cada ojo.
- Probé copias de imágenes en la memoria del servidor y comprobaciones de orden. No son copias hechas por la GPU de PS5.
- Comprimí y volví a leer vídeo real de prueba. La prueba funciona en este servidor; no captura juegos de PS5.

## Para qué sirve

Permite comprobar partes concretas y evita confundir una conexión o un dibujo de
prueba con compatibilidad real. También muestra por qué capturar solo las llamadas
`SubmitFlip` puede perder imágenes VR.

Una **interfaz** es el punto donde el juego pide un servicio al sistema. Queremos
sustituir esos servicios dentro del juego. Un **contrato** indica qué datos pide
cada servicio, qué devuelve y cuándo termina. La **GPU** es el chip que dibuja.
Necesitamos saber cuándo termina de dibujar antes de copiar la imagen.

## Qué falta

1. Comprobar los contratos exactos de las bibliotecas de tu versión 13.20.
2. Comprobar el inicio del modo VR sin un PS VR2 conectado.
3. Obtener las dos imágenes del juego y copiarlas sin que el juego las cambie durante la copia.
4. Comprimirlas en la PS5 con suficiente rapidez y mostrarlas en Quest.

Ninguna prueba de este trabajo se ejecutó en tu consola ni en tus Quest.
La prueba grande de vídeo del servidor no alcanzó 90 imágenes por segundo.
Reducir la resolución mejoró la velocidad, pero eso no demuestra cómo rendirá la PS5.

## Qué debes hacer tú

**Ahora no tienes que instalar ni ejecutar nada.** No hay un nuevo archivo listo
para probar la sustitución VR en la consola. Los programas nuevos son herramientas
de investigación para el servidor.

Si ya tienes un registro técnico de tu PS5 13.20, puedes compartir el texto después
de quitar contraseñas y direcciones privadas. Lo útil sería que muestre la versión
exacta de las bibliotecas VR y los resultados de sus llamadas. Un registro de carga
de un programa o una conexión de red, por sí solo, no aporta esos datos.

Si no tienes ese registro, no intentes crearlo con programas desconocidos. Primero
hace falta preparar y revisar una herramienta adecuada. El observador anterior
solo comprueba el arranque de su propio proceso; no recoge estos datos de VR.
No hay que comprar PS VR2, instalar Village ni repetir el inventario de tu equipo.

El [plan técnico de pruebas](PSVR2_NATIVE_EXPERIMENTS.md) detalla qué dato falta en
cada etapa. Los [resultados guardados](PSVR2_NATIVE_RESULTS.json) distinguen las
pruebas reales del servidor de las pruebas pendientes en PS5.
