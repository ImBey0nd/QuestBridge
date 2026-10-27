# Lo que tengo que hacer yo — QuestBridge

Actualizada: 27 de octubre de 2026. Esta guía es para el dueño del proyecto; no necesitas programar.

**Todavía no puedes jugar juegos de PS5 VR con QuestBridge.** Hay un prototipo de comunicación probado en PC, herramientas de investigación, una APK Android de diagnóstico y programas PS5 compilados. No hay una prueba real en Quest/PS5 ni sustitución de PS VR2 demostrada.

## 1. Tu equipo ya está registrado

No tienes que volver a completar la ficha. Confirmaste:

- PS5 Fat con firmware 13.20.
- [Kstuff Lite 1.11 Beta](https://github.com/EchoStretch/kstuff-lite/releases/tag/v1.11).
- Última versión de Relapse.
- Quest 2; el proyecto debe funcionar con la última versión de su sistema.
- Mandos originales de Quest 2.
- No tienes PS VR2 físico.
- Resident Evil 8 todavía no está instalado.

El objetivo es que una capa reutilizable permita jugar cualquier juego PS VR2
nativo de PS5. No necesitas instalar Resident Evil 8 ni comprar PS VR2 para que
continúe el desarrollo. La compatibilidad de cada juego se comprobará cuando
exista la integración; hoy ninguno está demostrado como compatible.

Seguiré investigando las interfaces PS5 con esos datos. Si una prueba concreta
requiere conocer el build exacto del cargador o un log, pediré solo ese dato y
explicaré para qué sirve. La herramienta JSON de inventario es opcional para el
desarrollo y no es una tarea que tengas que realizar.

**No cargues los ELF actuales en PS5.** El arranque del SDK cambia estado del
kernel antes de entrar al programa; siguen siendo artefactos de compilación.
No hay una prueba física nueva que debas ejecutar ahora.

### Contributors y copias antiguas del repositorio

Confirmaste que los commits que GitHub vinculaba a `ImNotBeyond` eran tuyos y autorizaste corregir su historial a `ImBey0nd`. La identidad de Codex usada en los trailers es `chatgpt-codex-connector[bot]`, no una cuenta humana llamada `codex`. GitHub calcula y actualiza la sección Contributors; no hay una lista manual que puedas editar y su caché puede tardar en reflejar la corrección.

Si tienes una copia antigua en tu PC, conserva cualquier cambio local y vuelve a clonar en una carpeta nueva después de la corrección. No fusiones ni vuelvas a subir las ramas antiguas, porque reintroducirían la atribución anterior. No necesitas hacerlo si solo usas GitHub y Codex. Se guarda un respaldo `.bundle` y un mapa de commits fuera del checkout en el entorno cloud; descárgalos si quieres conservar esa recuperación, ya que el entorno no es almacenamiento permanente. Las referencias históricas de evidencia se mantienen como fueron registradas.

## 2. Opcional: probar la APK de diagnóstico en mi Quest 2

Esta prueba es opcional y puede esperar mientras se investiga la integración PS5.

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
Cambios de equipo/versiones desde lo ya confirmado, solo si los hay:
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

Cuando esas condiciones estén cubiertas, prepararé una prueba física específica con pasos, resultados esperados y criterio para detenerse. Si hay comportamiento inesperado, se para y se conservan los registros. Hoy no hay una prueba de juego PS VR2 que puedas completar con QuestBridge.

No necesitas programar, adivinar APIs, compilar SDKs en tu PC ni comprar PS VR2 para que continúe el desarrollo. Las pruebas automáticas y builds se pueden seguir haciendo en cloud.
