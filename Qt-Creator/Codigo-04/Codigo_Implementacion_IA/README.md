# Primera etapa: Streaming + YOLOv11 (Python backend)

Esta base implementa la primera parte de tu consigna:
- Captura de video desde archivo, webcam o stream RTSP.
- Deteccion de vehiculos en tiempo real con YOLOv11.
- Visualizacion en vivo del video con bounding boxes.
- Salida JSON opcional para integracion con Qt mediante QProcess.

## 1) Preparar entorno

En terminal (PowerShell), dentro de la carpeta del proyecto:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

## 2) Ejecutar deteccion

### Con un archivo de video

```powershell
python backend\yolo_stream.py --source .\Video\autos.mp4
```

### Con stream RTSP

```powershell
python backend\yolo_stream.py --source "rtsp://usuario:password@ip:puerto/stream"
```

### Con webcam local (indice 0)

```powershell
python backend\yolo_stream.py --source 0
```

## 3) Modo integracion con Qt (salida JSON)

Si queres leer detecciones desde Qt, ejecuta:

```powershell
python backend\yolo_stream.py --source 0 --emit-json --no-show
```

Eso imprime una linea JSON por frame, por ejemplo:

```json
{"frame_id": 12, "timestamp": 1760000000.1, "detections": [{"class_id": 2, "label": "car", "confidence": 0.88, "bbox": [100, 120, 300, 260]}]}
```

## 4) Integracion minima con Qt Creator (idea)

1. Lanzar el script Python con QProcess.
2. Conectar la senal readyReadStandardOutput().
3. Leer linea por linea y parsear JSON (QJsonDocument).
4. Dibujar o listar detecciones en tu UI Qt.

## 5) Ejecutar desde Qt Creator (listo en este repo)

Se agrego una app Qt Widgets en `qt_app` para lanzar el backend Python sin pelear rutas.

La app Qt queda configurada para trabajar con qmake.

### Abrir en Qt Creator

1. Abri Qt Creator.
2. File > Open File or Project.
3. Selecciona `qt_app/CarlensQtRunner.pro`.
4. Configura el Kit `Qt 6.11.0 MinGW 64-bit` o equivalente.
5. Ejecuta el target `CarlensQtRunner`.

### Compilar con qmake desde terminal

El proyecto ya trae el `.pro` y un script PowerShell para compilar y desplegar las DLLs de Qt.

Ejemplo recomendado en Windows:

```powershell
.\qt_app\build_qmake.ps1
```

Para debug:

```powershell
.\qt_app\build_qmake.ps1 -Configuration debug
```

El ejecutable queda en `qt_app\build-qmake\bin\CarlensQtRunner.exe`.

Importante:
- Usa el kit MinGW que viene con Qt al compilar con qmake.
- Si tienes MSYS2 u otro `g++` antes en `PATH`, puedes terminar mezclando toolchains y la build puede fallar al linkear.
- El script `build_qmake.ps1` ya fuerza el `PATH` correcto y ejecuta `windeployqt` para copiar las DLLs necesarias junto al `.exe`.

### Sincronizacion remota de capturas

La app sigue guardando capturas localmente en `captures/`, pero ahora tambien puede:

1. subir la imagen al VPS por SSH/SFTP
2. crear `video_sources`
3. crear `processing_sessions`
4. crear `vehicle_tracks`
5. crear `vehicle_detections`
6. crear `vehicle_snapshots`

La configuracion sensible no va hardcodeada en el codigo. Debes crear este archivo local:

```text
config/vehicle_surveillance.remote.json
```

Puedes copiar como base:

```text
config/vehicle_surveillance.remote.example.json
```

Ese archivo debe contener:

1. credenciales MySQL
2. credenciales SSH al VPS
3. ruta base remota para snapshots
4. parametros por defecto de la sesion

Importante:

1. El archivo real `config/vehicle_surveillance.remote.json` esta ignorado por `.gitignore`.
2. La integracion remota usa un helper Python en `backend/remote_snapshot_sync.py`.
3. Debes instalar las nuevas dependencias Python de `requirements.txt` para habilitar MySQL y SFTP.
4. Si la config remota no existe o falla, la captura local sigue funcionando y la app mostrara un warning en la UI/log.

### Analisis de capturas con Gemini

La app puede analizar la captura confirmada con Gemini despues de guardarla, sin frenar el video.

Para habilitarlo:

1. Crea una API key en Google AI Studio.
2. Define la key en la variable de entorno `GEMINI_API_KEY` o en `config/vehicle_surveillance.remote.json`, dentro de `gemini.api_key`.
3. Deja `gemini.enabled=true`.

Configuracion recomendada inicial:

```json
"gemini": {
	"enabled": true,
	"api_key": "",
	"model": "gemini-2.5-flash-lite",
	"timeout_seconds": 30
}
```

Notas:

- El plan Gemini de estudiante no reemplaza la API key. Para la app necesitas una key de Google AI Studio.
- La primera implementacion guarda el resultado en `captures/<snapshot_id>.analysis.json` y lo muestra en el log de la app.

### Usar la app

- En "Fuente de video" deja `Video/autos.mp4` (o pone RTSP/ruta absoluta).
- Click en "Iniciar YOLO" para lanzar deteccion.
- Click en "Detener" para cerrar el proceso.
- El panel de log muestra stdout/stderr del backend.

Notas tecnicas:
- La app intenta ubicar automaticamente la raiz del proyecto buscando `backend/yolo_stream.py`.
- Usa `.venv/Scripts/python.exe` si existe; si no, usa `python` del sistema.
- Si desmarcas "Mostrar ventana de OpenCV", ejecuta con `--no-show --emit-json`.
- El backend Python sigue siendo el mismo; la migracion fue solo del sistema de build de Qt.

## Notas

- Modelo por defecto: yolo11n.pt (ligero y rapido para empezar).
- Clases detectadas: car, motorcycle, bus, truck.
- Tecla q o ESC para cerrar (cuando show esta activo).
