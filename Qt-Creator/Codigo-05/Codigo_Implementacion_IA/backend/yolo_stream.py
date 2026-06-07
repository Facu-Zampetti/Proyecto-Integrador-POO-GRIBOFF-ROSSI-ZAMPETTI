import argparse
import base64
import json
import os
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import cv2
from ultralytics import YOLO

# COCO classes used by YOLO: car=2, motorcycle=3, bus=5, truck=7
VEHICLE_CLASS_IDS = {2, 3, 5, 7}
CLASS_NAMES = {
    2: "car",
    3: "motorcycle",
    5: "bus",
    7: "truck",
}


def parse_source(source: str):
    """Convert source to int when webcam index is provided."""
    if source.isdigit():
        return int(source)
    return source


def should_throttle_source(source) -> bool:
    return isinstance(source, str) and Path(source).exists()


def is_rtsp_source(source) -> bool:
    return isinstance(source, str) and source.lower().startswith("rtsp://")


def is_network_source(source) -> bool:
    """Fuente de red en vivo (RTSP/RTMP/HTTP...). NO incluye archivos locales ni webcams."""
    if not isinstance(source, str) or should_throttle_source(source):
        return False
    return source.lower().startswith(
        ("rtsp://", "rtmp://", "http://", "https://", "udp://", "tcp://")
    )


def open_capture(source) -> cv2.VideoCapture:
    """Abre la captura endureciendo la ingesta para fuentes de red en vivo."""
    if is_rtsp_source(source):
        # Forzar RTSP sobre TCP: en redes con perdida (movil/WAN) el default UDP
        # produce cortes y artefactos. TCP es mas robusto a costa de algo de latencia.
        os.environ["OPENCV_FFMPEG_CAPTURE_OPTIONS"] = "rtsp_transport;tcp"
        cap = cv2.VideoCapture(source, cv2.CAP_FFMPEG)
    else:
        cap = cv2.VideoCapture(source)

    # Streams en vivo (no archivo local): minimizar el buffer para priorizar el
    # frame mas reciente y no acumular latencia (lag) cuando la red se atrasa.
    if not should_throttle_source(source):
        try:
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
        except Exception:
            pass
    return cap


def reconnect_with_backoff(
    source,
    base_delay: float = 1.0,
    max_delay: float = 5.0,
) -> Optional[cv2.VideoCapture]:
    """Reabre una fuente de red caida con backoff exponencial (reintentos infinitos).

    Pensado para microcortes de red movil: el proceso se detiene via SIGTERM desde
    Qt al desconectar, asi que reintentar indefinidamente es seguro y es lo que da
    la 'reconexion sola' sin tener que pulsar Desconectar/Conectar.
    """
    delay = base_delay
    attempt = 0
    while True:
        attempt += 1
        print(
            f"[reconnect] Fuente de red caida; reintentando en {delay:.0f}s "
            f"(intento {attempt})...",
            file=sys.stderr,
            flush=True,
        )
        time.sleep(delay)
        cap = open_capture(source)
        if cap.isOpened():
            ok, _ = cap.read()
            if ok:
                print("[reconnect] Reconexion exitosa.", file=sys.stderr, flush=True)
                return cap
        cap.release()
        delay = min(delay * 2.0, max_delay)


def draw_detection(frame, box: Tuple[int, int, int, int], label: str, conf: float):
    x1, y1, x2, y2 = box
    cv2.rectangle(frame, (x1, y1), (x2, y2), (40, 220, 40), 2)
    text = f"{label} {conf:.2f}"
    cv2.putText(
        frame,
        text,
        (x1, max(y1 - 10, 20)),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.6,
        (40, 220, 40),
        2,
        cv2.LINE_AA,
    )


def run_stream(
    source: str,
    model_path: str,
    conf: float,
    imgsz: int,
    show: bool,
    emit_json: bool,
):
    model = YOLO(model_path)
    parsed_source = parse_source(source)
    cap = open_capture(parsed_source)

    if not cap.isOpened():
        raise RuntimeError(f"No se pudo abrir la fuente de video: {source}")

    is_local_file_source = should_throttle_source(parsed_source)
    is_network = is_network_source(parsed_source)
    source_fps = cap.get(cv2.CAP_PROP_FPS)
    should_throttle = is_local_file_source and source_fps > 0.0
    frame_interval = (1.0 / source_fps) if should_throttle else 0.0
    next_frame_deadline = time.perf_counter()

    frame_id = 0
    t0 = time.time()

    while True:
        ok, frame = cap.read()
        if not ok:
            if is_local_file_source:
                # Archivo local: reiniciar (loop) al llegar al final.
                cap.release()
                cap = open_capture(parsed_source)
                if not cap.isOpened():
                    break

                next_frame_deadline = time.perf_counter()
                continue

            if is_network:
                # Fuente de red en vivo: un fallo de read() suele ser un microcorte
                # (WiFi/datos del telefono). Reabrir con backoff en vez de matar la
                # sesion, para que la reconexion sea automatica.
                cap.release()
                new_cap = reconnect_with_backoff(parsed_source)
                if new_cap is None:
                    break
                cap = new_cap
                continue

            break

        frame_id += 1
        display_frame = frame.copy()
        results = model(frame, conf=conf, imgsz=imgsz, verbose=False)

        detections: List[Dict] = []
        for r in results:
            if r.boxes is None:
                continue

            boxes = r.boxes.xyxy.cpu().numpy().astype(int)
            classes = r.boxes.cls.cpu().numpy().astype(int)
            scores = r.boxes.conf.cpu().numpy()

            for box, cls_id, score in zip(boxes, classes, scores):
                if cls_id not in VEHICLE_CLASS_IDS:
                    continue

                x1, y1, x2, y2 = box.tolist()
                label = CLASS_NAMES.get(cls_id, str(cls_id))

                detections.append(
                    {
                        "class_id": int(cls_id),
                        "label": label,
                        "confidence": float(score),
                        "bbox": [int(x1), int(y1), int(x2), int(y2)],
                    }
                )

                draw_detection(display_frame, (x1, y1, x2, y2), label, float(score))

        elapsed = time.time() - t0
        fps = frame_id / elapsed if elapsed > 0 else 0.0
        cv2.putText(
            display_frame,
            f"FPS: {fps:.1f} | Vehiculos: {len(detections)}",
            (20, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (30, 190, 255),
            2,
            cv2.LINE_AA,
        )

        if emit_json:
            ok_jpeg, encoded_frame = cv2.imencode(".jpg", frame)
            if not ok_jpeg:
                raise RuntimeError("No se pudo codificar el frame a JPEG")

            height, width = frame.shape[:2]
            payload = {
                "frame_id": frame_id,
                "timestamp": time.time(),
                "width": int(width),
                "height": int(height),
                "detections": detections,
                "frame_jpeg_base64": base64.b64encode(encoded_frame.tobytes()).decode("ascii"),
            }
            print(json.dumps(payload, ensure_ascii=True), flush=True)

        if show:
            cv2.imshow("YOLOv11 - Deteccion de Vehiculos", display_frame)
            key = cv2.waitKey(1) & 0xFF
            if key in (ord("q"), 27):
                break

        if should_throttle:
            next_frame_deadline += frame_interval
            remaining = next_frame_deadline - time.perf_counter()
            if remaining > 0:
                time.sleep(remaining)
            else:
                next_frame_deadline = time.perf_counter()

    cap.release()
    cv2.destroyAllWindows()


def build_parser():
    parser = argparse.ArgumentParser(
        description="Streaming de video + deteccion de vehiculos con YOLOv11"
    )
    parser.add_argument(
        "--source",
        required=True,
        help="Ruta a video, URL RTSP o indice de camara (0,1,...)"
    )
    parser.add_argument(
        "--model",
        default="yolo11n.pt",
        help="Modelo YOLOv11 (ejemplo: yolo11n.pt, yolo11s.pt)",
    )
    parser.add_argument(
        "--conf",
        type=float,
        default=0.35,
        help="Umbral de confianza",
    )
    parser.add_argument(
        "--imgsz",
        type=int,
        default=640,
        help="Tamano de imagen para inferencia",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Desactiva la ventana de visualizacion",
    )
    parser.add_argument(
        "--emit-json",
        action="store_true",
        help="Emite una linea JSON por frame (para Qt/QProcess)",
    )
    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    try:
        run_stream(
            source=args.source,
            model_path=args.model,
            conf=args.conf,
            imgsz=args.imgsz,
            show=not args.no_show,
            emit_json=args.emit_json,
        )
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
