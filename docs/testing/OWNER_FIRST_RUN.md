# Primera prueba física — QuestBridge

Este procedimiento prueba dos componentes por separado. **No permite jugar todavía.**
No requiere instalar Resident Evil 8, otro juego ni comprar PS VR2.

## Quest 2: cliente nuevo

Usa el paquete **questbridge-client-0.2.0-test-kit.zip**. Incluye APK, receptor PC,
fuente y LEEME.txt. Instala questbridge-client-debug.apk por ADB y abre
QuestBridge Diagnostics en Fuentes desconocidas. No uses la antigua APK que solo
consultaba el runtime.

Pulsa **Start stereo and tracking diagnostic**. Si funciona, verás un cubo verde,
una plataforma y marcadores azul/naranja para los mandos. Mueve cabeza y manos;
prueba menú del sistema, suspensión y reabrir. Detén la prueba si la imagen resulta
inestable o incómoda. Conserva el error exacto si la app se cierra.

La conexión opcional al receptor PC y captura de logs están en
[QUEST_CLIENT.md](QUEST_CLIENT.md). La escena y tracking por sí solos no prueban
integración con juegos PS5.

## PS5: un observador nuevo, en proceso independiente

El archivo es **questbridge-observer.elf**, 34 432 bytes, SHA256:

```text
da11946e155bdbab9ad18d810d3577856b6efe811d56d7a79d21f46f24fec661
```

Este candidato usa arranque propio; no enlaza el SDK CRT que hacía parches. Solo
comprueba su stub de entrada, obtiene su propio PID, escribe una línea y termina
su propio proceso. Compilación, inspección y 19 casos de emulación de CPU pasaron;
**no se ha ejecutado físicamente en PS5**. Tampoco recibe tracking ni llama APIs VR.

El perfil elegido es el servidor ELF independiente que trae el Relapse predeterminado
que reportaste, puerto 9021. La fuente pública y el binario incluido corroboran que
el camino de entrada ELF crea un proceso nuevo y redirige stdout a la conexión.
No hace falta repetir tu ficha. Una instalación o modo personalizado diferente
requiere revisión específica; no se asume compatible.

**No reemplaces el cargador ni los archivos de arranque de Relapse con este ELF.**
No lo uses con un inyector que seleccione un juego o proceso existente: el observador
termina el proceso que lo aloja. Los ELF anteriores del receptor y probe SDK siguen
siendo solo de compilación. Este candidato es un archivo distinto.

La prueba siguiente es manual y opcional para el dueño. Ninguna herramienta de
build/test de QuestBridge contacta la consola. Usa solamente tu cargador existente,
si ya está activo; este procedimiento no inicia exploits, instala cargadores,
cambia firmware ni modifica KStuff.

### Enviar una vez y guardar stdout desde Windows

Extrae el paquete del observador en una carpeta. Abre PowerShell allí. Copia este
bloque después de leerlo: al introducir la IPv4 local de tu PS5, envía ese ELF una
sola vez al cargador existente. No hay IP de consola preconfigurada.

```powershell
$path = (Resolve-Path .\questbridge-observer.elf).Path
$expected = 'da11946e155bdbab9ad18d810d3577856b6efe811d56d7a79d21f46f24fec661'
if ((Get-FileHash $path -Algorithm SHA256).Hash -ne $expected) {
    throw 'El hash no corresponde al observador revisado. No se enviara nada.'
}
$address = Read-Host 'IPv4 local de tu PS5 con el cargador independiente ya activo'
$client = New-Object System.Net.Sockets.TcpClient
$text = New-Object System.Text.StringBuilder
try {
    $connect = $client.ConnectAsync($address, 9021)
    if (-not $connect.Wait(5000)) { throw 'Tiempo de conexion agotado; no reintentes a ciegas.' }
    $stream = $client.GetStream()
    $stream.WriteTimeout = 5000
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $stream.Write($bytes, 0, $bytes.Length)
    $client.Client.Shutdown([System.Net.Sockets.SocketShutdown]::Send)
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    $buffer = New-Object byte[] 1024
    $total = 0
    while ($timer.ElapsedMilliseconds -lt 15000) {
        $stream.ReadTimeout = [Math]::Max(1, 15000 - [int]$timer.ElapsedMilliseconds)
        $count = $stream.Read($buffer, 0, $buffer.Length)
        if ($count -eq 0) { break }
        $total += $count
        if ($total -gt 16384) { throw 'Salida inesperadamente grande; prueba detenida.' }
        [void]$text.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $count))
    }
} finally {
    $client.Close()
    $text.ToString() | Set-Content -Encoding UTF8 .\questbridge-observer-output.txt
    $text.ToString()
}
```

Este bloque se proporciona para ejecución manual; no se ejecutó en Windows ni se
conectó a una consola durante el desarrollo cloud. La transferencia es TCP sin
cifrado dentro de tu red local; el observador no envía claves ni archivos de juegos.

La línea esperada es:

```text
QuestBridge observer v1: startup=custom; self_pid=positive; vr=unavailable
```

Conserva salida y error, incluso si no aparece esa línea. Si hay timeout, salida
vacía o error del cargador, detente y comparte lo observado; no repitas envíos ni
retires las comprobaciones. Una línea recibida demuestra que llegó al diagnóstico
y pudo escribir; no demuestra por sí sola una terminación limpia, syscall permissions
generales, sockets UDP, tracking aceptado o compatibilidad VR. Anota también si el
cargador y la consola siguieron respondiendo, según sus indicadores existentes.

## Qué devolver

- Quest: si apareció escena, si cabeza/mandos respondieron y qué pasó al suspender.
- PS5: línea o error exactos, fecha de la prueba y comportamiento observado del cargador.
- Logs revisados: sin claves, números de serie ni direcciones privadas.

No hacen falta módulos, juegos, dumps de firmware ni archivos Sony.
