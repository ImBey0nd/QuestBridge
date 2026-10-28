# Obtener evidencia de PS5 sin confundir una copia con una prueba de VR

Fecha: 2026-10-28. Objetivo: juegos ejecutados nativamente en PS5 Fat 13.20,
Quest 2 y Touch, sin PS VR2. Se mantiene el inventario ya confirmado.

El propietario confirma que tiene FTP y ha transferido archivos legibles en ambos
sentidos. Está fuera de casa; no se le pide encender la consola ahora. El nombre
y la versión del servidor siguen sin comprobarse. Que un archivo sea legible no
identifica su formato ni el procedimiento usado para obtenerlo: un texto normal
ya es legible y un ejecutable puede llegar descifrado por el servidor.

## Decisión práctica

Hay herramientas homebrew que transfieren archivos, descifran bibliotecas y leen
memoria. **No se ha verificado una herramienta única, limitada a lectura en toda
su cadena de ejecución, que recoja todos los contratos y las observaciones VR/GPU
necesarios en 13.20.** No equivale a demostrar que esa herramienta no pueda existir.

Primera opción: analizar copias que ya estén en el PC y descargar solamente
registros de texto ya existentes mediante el FTP que ya está funcionando. No
instalar otro servidor, no activar un depurador y no descargar automáticamente
todo el sistema. Esto obtiene evidencia parcial; no registra llamadas nuevas.

Segunda opción: desarrollar un observador específico con un alcance pequeño y
comprobar su arranque y sus operaciones antes de usarlo. No existe todavía un
recolector listo de contratos VR/GPU. El observador anterior de QuestBridge solo
comprueba su propio arranque. Un programa independiente no observa automáticamente
el proceso del juego; interceptar llamadas puede modificarlo o detenerlo y hay
que evaluar ese paso por separado. No llamaremos pasivo a un instalador que parchea.

## Herramientas homebrew contrastadas

[Fuentes primarias fijadas y comprobadas](PS5_EVIDENCE_COLLECTION_SOURCES.json).
Se revisaron 16 textos públicos comparándolos con sus identificadores Git exactos.
Los archivos fuente temporales quedaron fuera del proyecto. No se ejecutó ningún
programa en PS5 ni se descargó firmware, juegos o bibliotecas propietarias.

| Proyecto y revisión | Capacidad encontrada | Consecuencia para esta investigación |
| --- | --- | --- |
| [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv/tree/35bae1312b14840c0f95fab93be73d6daaa93bae) | FTP; `SELF` convierte SELF a ELF al descargar y está activado por defecto en esta revisión | Es una vía real para obtener bibliotecas analizables, pero no una copia sin efectos: el descifrado cambia temporalmente una tabla del núcleo y usa un archivo temporal |
| [websrv](https://github.com/ps5-payload-dev/websrv/tree/7748df9d2a43822a5ebfda149f43d4f2696d640b) | `/fs/` lista y sirve archivos con lectura ordinaria en el código revisado | Si ya está activo, puede transferir informes; no recoge por sí solo llamadas ni GPU. Arrancarlo de nuevo puede cambiar credenciales e instalar su lanzador |
| [ps5-self-pager](https://github.com/idlesauce/ps5-self-pager/tree/e98146660633c82766df46b77025d9b2d9c36597) | Su README anuncia extracción por carpetas y soporte 1.00–13.40 | El propio autor explica el cambio de tabla del núcleo y una ventana donde otro proceso podría fallar. No se recomienda como primer paso sin cambios |
| [ps5debug-NG](https://github.com/Pharaoh2k/ps5debug-NG/tree/a1a6cb2dae7544f892454f62046b7f70500ffa8e) | Enumera procesos/mapas y lee memoria; su README incluye 13.20 como «Fully Verified» | Es una afirmación de upstream. Su instalador puede aplicar un parche del núcleo, cambiar el filtro de llamadas e inyectarse en SceShellCore. Un cliente que elija solo leer no elimina esos pasos |
| [SDK público revisado](https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1) | Facilita construir programas homebrew | Su arranque llama a `__patch_init` antes de `main`. Un nuevo extractor no debe heredar ese arranque sin revisar, aunque su función principal solo imprima datos |

Detalles decisivos de ftpsrv: `cmd.c:674–695` abre el archivo para lectura, pero
selecciona el descifrado si `self2elf` está activo y el archivo es SELF;
`cmd.c:638–665` crea un temporal; `self-prospero.c:47–79` cambia/restaura la tabla
de paginación. `main-prospero.c` modifica AuthID al arrancar y puede terminar un
servidor previo. Su tabla contempla 13.20: presencia en fuente no prueba que el
binario instalado por el propietario sea ese ni que esté probado en su consola.

`SELF` es un interruptor por conexión, no una orden universal «desactivar». No
indicarlo a ciegas: clientes con reconexión o varias conexiones pueden usar estados
distintos. `MTRW` remonta particiones con permiso de escritura y no es necesario
para obtener estos informes. No se proporcionan comandos de parcheo, descifrado o
despliegue. No hace falta una copia entera del sistema ni una base de datos de claves.

Estos hallazgos proceden de código homebrew, no de una exigencia de usar herramientas
oficiales de Sony. El criterio es qué hace realmente la herramienta y qué demuestra
su resultado. Los proyectos no se certifican como seguros o inseguros para todo uso.

## Primera tanda, cuando haya copias en el computador

Se preparó [ps5-file-inventory.html](../../tools/diagnostics/ps5-file-inventory.html),
un comprobador que se abre localmente en el navegador, sin instalación. No es un
dumper de PS5. No hace peticiones de red ni ejecuta o descifra los archivos elegidos.
El informe descargable solo incluye nombres permitidos, tamaños, SHA-256 y un
resumen de cabecera; no incluye contenido, rutas locales, claves o direcciones.
Su política de contenido bloquea conexiones de red.

Pasos para el propietario:

1. Guardar y abrir el HTML en el computador con un navegador actualizado.
2. Elegir únicamente copias que ya existan de las bibliotecas indicadas en pantalla.
3. Pulsar «Crear informe» y después «Descargar informe».
4. Compartir `questbridge-inventario-local.json`; conservar las bibliotecas localmente.

Solo admite `libSceHmd2`, `libSceVrTracker2`, `libScePad`, `libSceVideoOut`,
`libSceAgcDriver` y `libkernel`, con extensión `.sprx`, `.prx` o `.elf`. No cambiar
el nombre de otro archivo para hacerlo pasar por uno de estos. Se rechazan
duplicados y selecciones mayores de seis archivos/64 MiB por archivo/256 MiB total.
Una biblioteca gráfica diferente podrá incorporarse cuando esté identificada.

SHA-256 es una huella de la copia local. Una cabecera ELF64 reconocida **no**
demuestra descifrado completo, procedencia, tablas internas válidas o un contrato
de llamadas correcto. Otra cabecera no significa necesariamente «cifrado».
Renombrar un SELF a `.elf` no lo convierte. Nuestro lector de imports solo admite
su perfil de ELF sin cifrar; no es un descifrador ni reconstruye automáticamente
las estructuras de Sony. Tampoco extrae por sí solo el contrato completo de exports.

Cuando se confirme el formato, las herramientas existentes pueden analizar en el
PC las importaciones de una copia autorizada y generar informes de texto. El
contenido propietario se queda fuera del repositorio y de los archivos a enviar.
Una importación revela qué función se referencia; no cómo está estructurado su
resultado ni si se llamó realmente. Eso exige análisis y observaciones adicionales.

## Primera tanda, mediante el FTP que ya está activo

Cuando el propietario vuelva a casa, primero se identifica el servidor con su
mensaje de bienvenida y versión. No se pide repetir el inventario de hardware.

1. Abrir el programa FTP habitual usando la conexión local existente.
2. Copiar el mensaje de bienvenida, quitando IP, usuario y credenciales.
3. Si ya se conocen registros de la aplicación o del cargador, descargar solo esos
   archivos normales de texto al PC. No se presupone que existan ni se inventa una ruta.
4. Revisar el texto antes de compartirlo y retirar claves, contraseñas e información
   de cuenta. Mantener los códigos de error, nombres de función y tiempos relevantes.
5. Compartir los informes seleccionados; cerrar la conexión al terminar.

No sincronizar PC hacia PS5, arrastrar carpetas del sistema, borrar, renombrar,
remontar particiones, leer dispositivos de `/dev` ni activar comandos especiales.
No abrir puertos del router para esto. No se afirma riesgo cero por usar FTP:
se limita la operación a recuperar texto de un servicio ya activo.

Registros de conexión FTP solo prueban transferencia. Si no contienen datos de VR,
no demuestran compatibilidad. Archivos legibles o un listado de bibliotecas tampoco
prueban que un juego pueda iniciar VR sin el dispositivo físico.

## Los datos necesarios, uno por uno

Los nombres de archivos de informe que se mencionan aquí son categorías de salida
que deberá generar el recolector, no archivos que ya existan en la consola.

### 1. Identidad exacta de las bibliotecas y del programa que las llama

**Qué:** nombres, tamaños, versión/identificador disponible y huella de Hmd2,
Tracker2, Pad, VideoOut y el proveedor gráfico realmente usado; versión del juego
o aplicación y entorno con que se compiló, cuando se pueda verificar.
**Cómo:** inventario local de copias existentes y, después, metadatos de módulos
cargados. Distinguir huella del archivo en disco de identidad del módulo en memoria.
**Para decidir:** evitar aplicar un contrato de otra revisión. Ya sabemos 13.20;
este dato no se deduce únicamente del número del firmware.

### 2. Dónde solicita VR el juego y cómo resuelve esas funciones

**Qué:** imports de la aplicación y sus bibliotecas del motor, proveedor de cada
función, resoluciones dinámicas, módulos cargados después y destino real al llamar.
**Cómo:** lector offline de imports más observación específica del proceso.
**Para decidir:** saber si el punto que sustituimos cubre al consumidor real.
Encontrar el nombre de una función o resolverla en nuestro proceso no basta.

### 3. Formato y reglas de cada llamada

**Qué:** argumentos, tamaños, valores devueltos, campos escritos, errores y reglas
por versión. Incluye retornos compuestos, memoria de trabajo y limpieza de recursos.
**Cómo:** análisis de fuentes/metadatos y del consumidor autorizado, contrastado con
registros acotados. No llamar funciones desconocidas con estructuras inventadas.
**Para decidir:** una respuesta puesta en el lugar equivocado puede provocar fallos.
Hay una discrepancia pendiente sobre la posición del tiempo en el estado del mando;
también existen consultas Tracker2 de 32 y 48 bytes que no se deben confundir.

### 4. Inicio y detección sin PS VR2

**Qué:** orden original de inicialización/apertura/detección, contexto de aplicación,
errores, salidas y servicios requeridos, con ausencia del visor documentada.
**Cómo:** registro existente o prueba pequeña con contratos ya verificados; para
un juego, evidencia de ese contexto. Un payload separado no representa al juego.
**Para decidir:** identificar qué servicios hay que sustituir. Si la inicialización
nativa falla, se descarta reenviarla sin más; no se descarta por ello todo visor virtual.

### 5. Posición y orientación de cabeza y mandos

**Qué:** unidades, ejes, origen/recentrado, poses, estado de seguimiento, confianza,
casos de pérdida y desconexión, y formato exacto que lee el consumidor.
**Cómo:** datos reales de Quest comparados con el contrato del receptor y sus
consultas. No hace falta obtener cámaras o imágenes del entorno del propietario.
**Para decidir:** que el juego mueva la vista/manos correctamente y sepa cuándo
un dato dejó de ser válido. Números de pose plausibles no equivalen a seguimiento aceptado.

### 6. Botones, gatillos y estado de los controladores

**Qué:** aperturas/tipos/identificadores de mandos, bits de botones, palancas,
gatillos, conexión, tiempos, consultas adicionales y efectos solicitados.
**Cómo:** contrato de Pad y registro del consumidor; traducir los datos Touch que
realmente existen. El cuerpo de una función sin implementar no verifica su formato.
**Para decidir:** representar las entradas necesarias. Sustituir botones no aporta
por sí solo manos con posición; eso viene del seguimiento del punto anterior.

### 7. Relojes, predicción y esperas de presentación

**Qué:** unidades y relación entre reloj PS5/Quest, tiempo pedido por el juego,
cuadro, pose utilizada al renderizar, frecuencia, BeginFrame y eventos esperados.
**Cómo:** tiempos correlacionados y medición de ida/vuelta/retraso. No identificar
el momento de llegada de un paquete con el momento para el que se predijo la pose.
**Para decidir:** mantener la simulación y presentación estables sin depender de
una pantalla física ausente. Un juego que espera un evento que nunca emitimos puede pararse.

### 8. Identidad y formato de las dos imágenes

**Qué:** registro de buffers, identificadores de puerto/conjunto/índice/generación,
ambas variantes SetParam, capas, rectángulos de cada ojo, descriptores de textura,
resolución/formato/color y relación con cuadro, pose y tiempo.
**Cómo:** metadatos acotados al registrar/enviar y asociación observada, conservando
el resultado original. Un descriptor es una ficha de la imagen, no sus píxeles.
**Para decidir:** localizar ambas vistas reales, también si no hay SubmitFlip directo.
Un espejo de TV, una dirección o un listado de texturas no prueba captura estéreo.

### 9. Cuándo termina la GPU y cómo copiar/liberar correctamente

**Qué:** dependencia que indica finalización de dibujo, visibilidad de memoria,
organización/compresión de textura, resolve/detile necesarios, destino propio,
final de copia y momentos de reutilización/desregistro y retirada del cuadro.
**Cómo:** evidencia de la cola gráfica y prueba de dos ojos distinguibles, incluidas
reutilización rápida, fallos y esperas agotadas. No basta decir «fence listo» sin identidad.
**Para decidir:** evitar copiar imágenes incompletas o memoria ya reutilizada. El
reproductor público revisado puede continuar tras agotarse su espera sin confirmar finalización.

### 10. Compresión de vídeo viable en la PS5

**Qué:** codificador realmente accesible, formato de entrada/salida, propiedad de
memoria, cola, envío/recepción/vaciado/errores, recursos consumidos y tiempos reales.
**Cómo:** documentación/código utilizable y prueba nativa primero con patrones
propios, después compartiendo recursos con el consumidor nativo. No inferir una
interfaz disponible solo por saber que existe Remote Play o grabación en el sistema.
**Para decidir:** enviar ambas vistas a una resolución/frecuencia acordadas sin
acumular retraso. El resultado de x264 del servidor no mide el codificador de PS5.

### 11. Funciones opcionales u obligatorias del juego

**Qué:** detección de visor puesto, área de juego/calibración, mirada, vibración y
consultas de capacidades que usa el título, con opción real de desactivarlas si existe.
**Cómo:** consumidor y secuencia de configuración, sin presentar respuestas vacías
como funciones disponibles. Quest 2 no dispone de seguimiento ocular.
**Para decidir:** declarar un subconjunto soportado. La incompatibilidad de una
función obligatoria puede excluir un título; no prueba que todos los demás sean inviables.

### 12. Funcionamiento completo, retraso y recuperación

**Qué:** título nativo funcionando sin PS VR2, dos vistas correctas en Quest,
movimiento y mandos, pérdida de seguimiento/red, reconexión, cierre y recursos
liberados; medición desde movimiento hasta imagen y no solo velocidad de red.
**Cómo:** prueba final reproducible de la ruta completa, con versiones y resultados.
**Para decidir:** es la prueba de sustitución real. Una conexión, una escena propia
o un único título no demuestra compatibilidad universal. No se requiere comprar
PS VR2 ni un juego específico para desarrollar las etapas anteriores.

## Si no podemos obtenerlo todo con el FTP existente

- Analizar las copias y registros ya disponibles y compartir solo metadatos. Permite
  avanzar en identidad/imports/contratos candidatos sin otra ejecución en consola.
- Construir un observador mínimo original con arranque revisado, operaciones
  verificadas y salida acotada; inicialmente dentro de su propia aplicación. Primero
  metadatos, después la primera llamada concreta que falte. No un volcado general.
- Separar una eventual instrumentación del juego: cambios de memoria, pausa,
  instalación o privilegios deben describirse como tales antes de una prueba.
- Contrastar registros primarios de otros desarrolladores con la misma revisión;
  sirven para reducir incertidumbre, no para declarar probada la consola del propietario.

El fracaso de una herramienta o la ausencia de un API en una búsqueda no refutan
por sí solos el método. Cada resultado debe identificar qué variante descarta y
qué alternativa sigue abierta. El [plan de experimentos](../testing/PSVR2_NATIVE_EXPERIMENTS.md)
conserva los criterios técnicos C1–C6. Las pruebas restantes requieren esas
observaciones concretas, no una copia indiscriminada de archivos personales.

## Verificación realizada del comprobador local

`npm test`: 37/37; CTest existente: 8/8, incluido el codificador real del servidor.
Las cuatro pruebas nuevas ejecutan el JavaScript exacto del HTML: huellas de bytes
seleccionados, límites antes de leer, cabeceras truncadas/incompatibles y ausencia
de exportación de rutas/contenido o declaraciones de compatibilidad.

`node tools/diagnostics/test-file-inventory.mjs` comprobó con Chromium 154 la
selección de un archivo inventado, su informe/huella y la descarga del JSON.
No se usaron bibliotecas reales. El navegador administrado bloqueó inicialmente
las URL `file:` con `ERR_BLOCKED_BY_ADMINISTRATOR`; la prueba funcional usó el
HTML idéntico servido temporalmente por HTTP en loopback con ruta aleatoria de
256 bits como clave. Se cerraron navegador/servidor y se borraron sus temporales.
No había puerto de depuración: DevTools se comunicó por pipe local.

Por tanto está probada la lógica y la interfaz servida localmente; no se afirma
haber probado la apertura `file:` en este navegador administrado ni en el PC del
propietario. No hay que desactivar protecciones si su navegador impide abrirlo.
El servidor temporal y la opción de aislamiento del navegador son solo del
ensayo automatizado; el comprobador distribuido no contiene un servidor ni pide
activar opciones especiales. Ninguna de estas pruebas accedió a la PS5.
