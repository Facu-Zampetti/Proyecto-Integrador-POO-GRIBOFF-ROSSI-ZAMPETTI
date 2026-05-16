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

### Abrir en Qt Creator

1. Abri Qt Creator.
2. File > Open File or Project.
3. Selecciona `qt_app/CMakeLists.txt`.
4. Configura un Kit con Qt 6 y compila.
5. Ejecuta el target `CarlensQtRunner`.

### Usar la app

- En "Fuente de video" deja `Video/autos.mp4` (o pone RTSP/ruta absoluta).
- Click en "Iniciar YOLO" para lanzar deteccion.
- Click en "Detener" para cerrar el proceso.
- El panel de log muestra stdout/stderr del backend.

Notas tecnicas:
- La app intenta ubicar automaticamente la raiz del proyecto buscando `backend/yolo_stream.py`.
- Usa `.venv/Scripts/python.exe` si existe; si no, usa `python` del sistema.
- Si desmarcas "Mostrar ventana de OpenCV", ejecuta con `--no-show --emit-json`.

## Notas

- Modelo por defecto: yolo11n.pt (ligero y rapido para empezar).
- Clases detectadas: car, motorcycle, bus, truck.
- Tecla q o ESC para cerrar (cuando show esta activo).
