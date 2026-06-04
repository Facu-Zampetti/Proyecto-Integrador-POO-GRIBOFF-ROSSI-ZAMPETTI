import argparse
import base64
import json
import mimetypes
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path


DEFAULT_MODEL = "gemini-2.5-flash-lite"
DEFAULT_TIMEOUT_SECONDS = 30


def load_json_file(path: str):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def determine_mime_type(image_path: str) -> str:
    guessed, _ = mimetypes.guess_type(image_path)
    return guessed or "image/jpeg"


def resolve_settings(config: dict) -> dict:
    gemini = config.get("gemini", {})
    api_key = os.environ.get("GEMINI_API_KEY") or os.environ.get("GOOGLE_API_KEY") or gemini.get("api_key", "")
    enabled = bool(api_key) and gemini.get("enabled", True)
    return {
        "enabled": enabled,
        "api_key": api_key,
        "model": gemini.get("model", DEFAULT_MODEL),
        "timeout_seconds": int(gemini.get("timeout_seconds", DEFAULT_TIMEOUT_SECONDS)),
    }


def build_prompt(snapshot_id: str, detected_label: str) -> str:
    return (
        "Analiza la imagen de un vehiculo y responde SOLO en JSON valido. "
        "Si no puedes inferir un dato con suficiente certeza, usa null. "
        "Campos requeridos: make, model, color, body_type, vehicle_type, year_range, confidence, summary. "
        "confidence debe ser un numero entre 0 y 1. "
        f"Snapshot: {snapshot_id}. Deteccion original: {detected_label or 'vehicle'}. "
        "No agregues markdown, no agregues texto fuera del JSON."
    )


def extract_text_response(response_json: dict) -> str:
    candidates = response_json.get("candidates", [])
    if not candidates:
        raise RuntimeError("Gemini no devolvio candidates.")

    content = candidates[0].get("content", {})
    parts = content.get("parts", [])
    if not parts:
        raise RuntimeError("Gemini no devolvio partes de contenido.")

    text = parts[0].get("text", "").strip()
    if not text:
        raise RuntimeError("Gemini devolvio una respuesta vacia.")

    return text


def normalize_analysis(raw_analysis: dict) -> dict:
    confidence = raw_analysis.get("confidence")
    try:
        normalized_confidence = max(0.0, min(1.0, float(confidence))) if confidence is not None else None
    except (TypeError, ValueError):
        normalized_confidence = None

    return {
        "make": raw_analysis.get("make"),
        "model": raw_analysis.get("model"),
        "color": raw_analysis.get("color"),
        "body_type": raw_analysis.get("body_type"),
        "vehicle_type": raw_analysis.get("vehicle_type"),
        "year_range": raw_analysis.get("year_range"),
        "confidence": normalized_confidence,
        "summary": raw_analysis.get("summary"),
    }


def request_gemini_analysis(settings: dict, image_path: str, prompt: str) -> dict:
    image_bytes = Path(image_path).read_bytes()
    endpoint = (
        f"https://generativelanguage.googleapis.com/v1beta/models/"
        f"{settings['model']}:generateContent?key={settings['api_key']}"
    )

    payload = {
        "contents": [
            {
                "parts": [
                    {
                        "inline_data": {
                            "mime_type": determine_mime_type(image_path),
                            "data": base64.b64encode(image_bytes).decode("ascii"),
                        }
                    },
                    {"text": prompt},
                ]
            }
        ],
        "generationConfig": {
            "temperature": 0.2,
            "responseMimeType": "application/json",
        },
    }

    request = urllib.request.Request(
        endpoint,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    try:
        with urllib.request.urlopen(request, timeout=settings["timeout_seconds"]) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"Gemini API devolvio HTTP {exc.code}: {detail}") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"No se pudo conectar con Gemini: {exc}") from exc


def write_output_file(output_path: str, payload: dict):
    if not output_path:
        return

    output = Path(output_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def build_parser():
    parser = argparse.ArgumentParser(description="Analiza una captura de vehiculo usando Gemini")
    parser.add_argument("--config", required=True, help="Ruta al archivo JSON de configuracion")
    parser.add_argument("--image", required=True, help="Ruta local de la imagen a analizar")
    parser.add_argument("--snapshot-id", required=True, help="Identificador de la captura")
    parser.add_argument("--label", default="", help="Etiqueta detectada por YOLO")
    parser.add_argument("--output", default="", help="Ruta opcional para guardar el resultado JSON")
    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    try:
        config = load_json_file(args.config)
        settings = resolve_settings(config)
        if not settings["enabled"]:
            print(
                json.dumps(
                    {
                        "success": False,
                        "configured": False,
                        "error": (
                            "Gemini no esta configurado. Crea una API key en Google AI Studio y "
                            "definela en GEMINI_API_KEY o en config/vehicle_surveillance.remote.json."
                        ),
                    },
                    ensure_ascii=True,
                )
            )
            return

        if not Path(args.image).exists():
            print(
                json.dumps(
                    {
                        "success": False,
                        "configured": True,
                        "error": f"No existe la imagen a analizar: {args.image}",
                    },
                    ensure_ascii=True,
                )
            )
            return

        prompt = build_prompt(args.snapshot_id, args.label)
        response_json = request_gemini_analysis(settings, args.image, prompt)
        analysis_text = extract_text_response(response_json)
        raw_analysis = json.loads(analysis_text)
        analysis = normalize_analysis(raw_analysis)

        output_payload = {
            "snapshot_id": args.snapshot_id,
            "model": settings["model"],
            "analysis": analysis,
        }
        write_output_file(args.output, output_payload)

        print(
            json.dumps(
                {
                    "success": True,
                    "configured": True,
                    "output_path": args.output,
                    "analysis": analysis,
                },
                ensure_ascii=True,
            )
        )
    except Exception as exc:
        print(json.dumps({"success": False, "configured": True, "error": str(exc)}, ensure_ascii=True))
        sys.exit(1)


if __name__ == "__main__":
    main()