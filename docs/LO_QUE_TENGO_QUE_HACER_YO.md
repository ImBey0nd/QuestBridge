# Lo que tengo que hacer yo — QuestBridge

Fecha: 26 de octubre de 2026. Esta guía es para el dueño del proyecto; no necesitas programar.

**Todavía no puedes jugar juegos de PS5 VR con QuestBridge.** Hay un prototipo de comunicación probado en PC, herramientas de investigación, una APK Android de diagnóstico y programas PS5 compilados. No hay una prueba real en Quest/PS5 ni sustitución de PS VR2 demostrada.

## 1. Primero: anotar mi PS5, sin instalar nada

Tu siguiente tarea prioritaria es rellenar esta ficha mirando la información del sistema y los registros que ya tengas. No actualices firmware, Relapse o KStuff para completar la ficha. Si no sabes algo, escribe «no sé».

```text
Fecha y zona horaria:
Modelo exacto de PS5 (no número de serie):
Versión completa de firmware que muestra la consola:
Versión instalada de Relapse (si se conoce):
Versión instalada de KStuff (si se conoce):
Nombre y versión del cargador ELF que utilizo:
¿Ese cargador permite ver/guardar mensajes stdout?:
Quest: modelo y versión del sistema:
Mandos que tengo:
¿Tengo PS VR2 físico disponible? (no hace falta comprarlo):
Resident Evil Village: región, versión y VR Mode instalado:
Identificador del juego, solo si está verificado:
```

Mándame esa ficha y cualquier log existente del cargador, quitando direcciones privadas, números de serie y credenciales. Que upstream tenga una versión nueva no significa que esté instalada ni probada en tu consola. Detalles: [HARDWARE_NEXT.md](testing/HARDWARE_NEXT.md).

**No cargues los ELF actuales en PS5.** Se descubrió que el arranque del SDK cambia estado del kernel antes de entrar al programa. Ambos artefactos PS5 siguen siendo solo de compilación. Primero hace falta resolver y revisar ese arranque y el contrato del cargador. No hay un puerto/IP de PS5 que debas configurar ahora.

### Contributors y copias antiguas del repositorio

Confirmaste que los commits que GitHub vinculaba a `ImNotBeyond` eran tuyos y autorizaste corregir su historial a `ImBey0nd`. La identidad de Codex usada en los trailers es `chatgpt-codex-connector[bot]`, no una cuenta humana llamada `codex`. GitHub calcula y actualiza la sección Contributors; no hay una lista manual que puedas editar y su caché puede tardar en reflejar la corrección.

Si tienes una copia antigua en tu PC, conserva cualquier cambio local y vuelve a clonar en una carpeta nueva después de la corrección. No fusiones ni vuelvas a subir las ramas antiguas, porque reintroducirían la atribución anterior. No necesitas hacerlo si solo usas GitHub y Codex. Se guarda un respaldo `.bundle` y un mapa de commits fuera del checkout en el entorno cloud; descárgalos si quieres conservar esa recuperación, ya que el entorno no es almacenamiento permanente. Las referencias históricas de evidencia se mantienen como fueron registradas.

## 2. Opcional: probar la APK de diagnóstico en mi Quest 2

Puedes hacerlo después de la ficha PS5. Si prefieres seguir primero con PS5, esta prueba puede esperar.

### Preparar el visor y obtener la APK

1. Carga el Quest y prepara un cable USB de datos y un PC. Esta parte física necesita tu PC; el desarrollo puede continuar en cloud.
2. Habilita el modo desarrollador del visor mediante las opciones oficiales de Meta para tu cuenta. Instala Android Platform Tools desde [Google](https://developer.android.com/tools/releases/platform-tools). En Windows, instala el controlador USB oficial de Meta si ADB no detecta el visor.
3. En [GitHub Actions](https://github.com/ImBey0nd/QuestBridge/actions), abre una ejecución verde de **QuestBridge checks** correspondiente al código integrado. Descarga el artefacto **questbridge-probe-debug** y extrae `app-debug.apk`. Puede requerir iniciar sesión. Si el artefacto expiró, usa **Run workflow** sobre `main` y espera a que termine en verde.
4. Guarda la URL de esa ejecución y su commit. No uses un APK de procedencia desconocida ni copies el hash de otra compilación: cada entorno puede firmar distinto. El APK histórico de cloud y sus hashes están en [CLOUD_2026-10-26.md](testing/CLOUD_2026-10-26.md).
5. Conecta el visor por USB, póntelo y acepta la autorización de depuración para tu PC.

### Instalar, abrir y guardar los resultados

Abre PowerShell en la carpeta de Platform Tools. Coloca allí la APK extraída como `app-debug.apk`. Los comandos siguientes son para Windows. En Linux/macOS usa `adb` en lugar de `.\adb.exe`.

```powershell
.\adb.exe devices
Get-FileHash .\app-debug.apk -Algorithm SHA256
.\adb.exe install -r .\app-debug.apk
```

Debe aparecer **un solo visor** con estado `device`. Si dice `unauthorized`, acepta el aviso dentro del visor. Si hay varios dispositivos, desconecta los demás antes de continuar. El listado contiene el número de serie: no lo publiques.

Si la instalación falla, guarda el mensaje exacto y detente. Si dice que la firma es incompatible con una instalación anterior, no desinstales a ciegas: envíame el error para decidir el siguiente paso.

Antes de abrir la aplicación, inicia la captura en una ventana PowerShell:

```powershell
.\adb.exe logcat -v threadtime QuestBridge:I OpenXR-Loader:V '*:S' > .\questbridge-logcat.txt
```

Déjala abierta. En una segunda ventana en la misma carpeta:

```powershell
.\adb.exe shell am start -W -n org.questbridge.probe/.MainActivity
```

Lee el texto que aparece en la app. El resultado esperado, **si funciona**, incluye `Runtime: ...; HMD system discovered. No XR session or tracking started.` Una pantalla de texto es normal; esta app no dibuja un entorno VR.

Después:

1. Copia o fotografía el resultado completo, incluso si es un error.
2. Cierra la app y vuelve a abrirla. Anota si cambia el resultado o se cierra sola.
3. Suspende y despierta el visor; vuelve a abrir la app y anota qué ocurre. No esperes mensajes nuevos automáticamente al despertar: el probe se ejecuta al crear la actividad.
4. Detén la captura con `Ctrl+C` en la primera ventana.
5. Guarda el hash SHA256, versión del sistema del Quest, commit/URL del build, resultado de instalación, texto de pantalla y `questbridge-logcat.txt`.

Si la app se cierra y el archivo no explica por qué, captura localmente `adb logcat -d -b crash` y elimina datos ajenos antes de compartirlo. No cambies firmware ni instales runtimes de terceros intentando arreglar un error.

**Pasar esta prueba solo demuestra descubrimiento OpenXR.** La APK no envía tracking, no prueba mandos, no recibe vídeo ni se conecta a PS5.

## 3. Qué devolverme para que yo pueda seguir

```text
Ficha PS5 de la sección 1:
Prueba Quest realizada: sí / no
URL de GitHub Actions y commit del APK:
SHA256 del APK instalado:
Resultado de adb install y de am start:
Texto exacto en pantalla:
Resultado al cerrar/abrir y suspender/despertar:
Logs adjuntos, revisados para quitar datos privados:
```

Los logs de fallos también sirven. No subas claves, contraseñas, binarios de Sony, juegos, dumps de firmware ni números de serie al repositorio público. No necesitas enviarme los archivos de módulos o del juego.

## 4. Qué falta antes de probar un juego real

Estas etapas requieren más trabajo de desarrollo e investigación; no son instrucciones para que ahora intentes ejecutarlas:

| Etapa | Qué hay que verificar antes de avanzar |
| --- | --- |
| Arranque PS5 | Un artefacto revisado para tu cargador, sin escrituras automáticas al kernel, con hash y logs de arranque |
| Receptor PS5 | Autenticación, tracking sintético, reconexión y neutralización de datos vencidos, comprobados en consola |
| Interfaz VR nativa | Detección, inicialización, poses, mandos y acceso a estéreo reales con ABI justificada para tu firmware |
| Cliente Quest | Sesión OpenXR, tracking de cabeza/manos, mandos, calibración y escena de diagnóstico reales |
| Vídeo | Vídeo estéreo nativo, latencia, pérdidas y sincronización medidos; el vídeo sintético no acredita compatibilidad |
| Primer juego | Versión/región exactas, inicio VR, imagen por ojo, respuesta a cabeza/mandos y registros reproducibles |

Cuando esas condiciones estén cubiertas, prepararé una prueba física específica con pasos, resultados esperados y criterio para detenerse. Si hay comportamiento inesperado, se para y se conservan los registros. Hoy no hay una prueba de Resident Evil Village que puedas completar con QuestBridge.

No necesitas programar, adivinar APIs, compilar SDKs en tu PC ni comprar PS VR2 para avanzar con la ficha inicial. Las pruebas automáticas y builds se pueden seguir haciendo en cloud.
