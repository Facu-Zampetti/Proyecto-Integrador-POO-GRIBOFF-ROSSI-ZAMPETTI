import argparse
import contextlib
import json
import logging
import os
import posixpath
import sys
import zlib
from datetime import datetime, timedelta, timezone
from pathlib import Path

import pymysql.err

MYSQL_SIGNED_INT_MAX = 2_147_483_647

# Argentina usa UTC-3 todo el año (sin horario de verano).
ARGENTINA_UTC_OFFSET = timedelta(hours=-3)


def load_optional_dependency(module_name: str, package_name: str):
    try:
        return __import__(module_name)
    except ImportError as exc:
        raise RuntimeError(
            f"Falta la dependencia Python '{package_name}'. Ejecuta 'pip install -r requirements.txt'."
        ) from exc


def load_json_file(path: str):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds")


def to_mysql_datetime_string(dt: datetime) -> str:
    if dt.tzinfo is not None:
        dt = dt.astimezone(timezone.utc).replace(tzinfo=None)
    return dt.strftime("%Y-%m-%d %H:%M:%S")


def normalize_timestamp(value: str | None) -> str:
    if not value:
        return to_mysql_datetime_string(datetime.now(timezone.utc))

    normalized = value.strip()
    try:
        parsed = datetime.fromisoformat(normalized.replace("Z", "+00:00"))
        return to_mysql_datetime_string(parsed)
    except ValueError:
        pass

    for fmt in ("%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M:%S.%f"):
        try:
            parsed = datetime.strptime(normalized, fmt)
            return to_mysql_datetime_string(parsed)
        except ValueError:
            continue

    raise RuntimeError(
        f"Formato de fecha no soportado para MySQL: {value}. Usa ISO-8601 o 'YYYY-MM-DD HH:MM:SS'."
    )


def parse_optional_nonnegative_int(value) -> int | None:
    if value is None:
        return None
    if isinstance(value, bool):
        return None
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return None
    return parsed if parsed >= 0 else None


def fit_external_track_id(raw_value: int) -> int:
    if 0 <= raw_value <= MYSQL_SIGNED_INT_MAX:
        return raw_value
    return zlib.crc32(str(raw_value).encode("utf-8")) & MYSQL_SIGNED_INT_MAX


def build_synthetic_external_track_id(snapshot: dict) -> int:
    seed_parts = [
        str(snapshot.get("snapshot_id") or ""),
        str(snapshot.get("frame_id") or ""),
        str(snapshot.get("captured_at_utc") or ""),
        str(snapshot.get("bbox") or ""),
        str(snapshot.get("label") or ""),
    ]
    seed = "|".join(seed_parts)
    return zlib.crc32(seed.encode("utf-8")) & MYSQL_SIGNED_INT_MAX


def infer_source_name(source_path: str) -> str:
    source = source_path.strip()
    if source.isdigit():
        return f"camera_{source}"
    if source.lower().startswith("rtsp://"):
        return source.split("@")[-1]
    return Path(source).name or "video_source"


def require_config_fields(config: dict):
    database = config.get("database", {})
    vps = config.get("vps", {})
    snapshots = config.get("snapshots", {})

    required_db_fields = ["name", "user", "password"]
    missing_db = [field for field in required_db_fields if not database.get(field)]
    if missing_db:
        raise RuntimeError(f"Faltan campos de database en config: {', '.join(missing_db)}")

    if not database.get("host"):
        database["host"] = vps.get("host") or "127.0.0.1"
    if not database.get("port"):
        database["port"] = 3306

    ssh_tunnel = database.get("ssh_tunnel", {})
    if database.get("use_ssh_tunnel") is None:
        database["use_ssh_tunnel"] = True
    if not ssh_tunnel.get("remote_host"):
        ssh_tunnel["remote_host"] = "127.0.0.1"
    if not ssh_tunnel.get("remote_port"):
        ssh_tunnel["remote_port"] = int(database.get("port", 3306))
    database["ssh_tunnel"] = ssh_tunnel

    required_vps_fields = ["host", "port", "username", "auth_method"]
    missing_vps = [field for field in required_vps_fields if not vps.get(field)]
    if missing_vps:
        raise RuntimeError(f"Faltan campos de vps en config: {', '.join(missing_vps)}")

    auth_method = str(vps.get("auth_method", "")).lower()
    if auth_method == "password" and not vps.get("password"):
        raise RuntimeError("El auth_method=password requiere vps.password en la config.")
    if auth_method == "key" and not vps.get("private_key_path"):
        raise RuntimeError("El auth_method=key requiere vps.private_key_path en la config.")

    if not snapshots.get("base_path"):
        raise RuntimeError("Falta snapshots.base_path en la config.")


def create_ssh_connect_kwargs(vps: dict) -> dict:
    auth_method = str(vps.get("auth_method", "key")).lower()
    connect_kwargs = {
        "ssh_address_or_host": (vps["host"], int(vps.get("port", 22))),
        "ssh_username": vps["username"],
    }
    if auth_method == "password":
        connect_kwargs["ssh_password"] = vps["password"]
    else:
        connect_kwargs["ssh_pkey"] = vps["private_key_path"]
        if vps.get("password"):
            connect_kwargs["ssh_private_key_password"] = vps["password"]
    return connect_kwargs


def get_quiet_sshtunnel_logger():
    logger = logging.getLogger("vehicle_remote_sync.sshtunnel")
    logger.setLevel(logging.CRITICAL)
    logger.propagate = False
    if not logger.handlers:
        logger.addHandler(logging.NullHandler())
    return logger


def open_ssh_client(config: dict):
    paramiko = load_optional_dependency("paramiko", "paramiko")

    vps = config["vps"]
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    raw_connect_kwargs = create_ssh_connect_kwargs(vps)
    connect_kwargs = {
        "hostname": raw_connect_kwargs["ssh_address_or_host"][0],
        "port": raw_connect_kwargs["ssh_address_or_host"][1],
        "username": raw_connect_kwargs["ssh_username"],
        "timeout": 15,
    }
    if "ssh_password" in raw_connect_kwargs:
        connect_kwargs["password"] = raw_connect_kwargs["ssh_password"]
    else:
        connect_kwargs["key_filename"] = raw_connect_kwargs["ssh_pkey"]
        if raw_connect_kwargs.get("ssh_private_key_password"):
            connect_kwargs["passphrase"] = raw_connect_kwargs["ssh_private_key_password"]

    ssh_client.connect(**connect_kwargs)
    return ssh_client


def run_remote_command(config: dict, command: str) -> str:
    ssh_client = open_ssh_client(config)
    try:
        _, stdout, stderr = ssh_client.exec_command(command, timeout=15)
        exit_status = stdout.channel.recv_exit_status()
        output = stdout.read().decode("utf-8", errors="replace").strip()
        error = stderr.read().decode("utf-8", errors="replace").strip()
        if exit_status != 0:
            raise RuntimeError(error or f"Fallo el comando remoto: {command}")
        return output
    finally:
        ssh_client.close()


def discover_mysql_container_host(config: dict) -> str | None:
    tunnel_config = config.get("database", {}).get("ssh_tunnel", {})
    container_name = tunnel_config.get("docker_container_name")
    if not container_name:
        candidates_output = run_remote_command(
            config,
            "docker ps --format '{{.Names}}|{{.Image}}'",
        )
        for line in candidates_output.splitlines():
            name, _, image = line.partition("|")
            if "mysql" in image.lower() or "-db-" in name.lower() or name.lower().endswith("db-1"):
                container_name = name.strip()
                break

    if not container_name:
        return None

    output = run_remote_command(
        config,
        f"docker inspect -f '{{{{range .NetworkSettings.Networks}}}}{{{{.IPAddress}}}}{{{{end}}}}' {container_name}",
    )
    return output.strip() or None


def start_ssh_tunnel(config: dict, remote_host: str, remote_port: int):
    sshtunnel = load_optional_dependency("sshtunnel", "sshtunnel")
    tunnel = sshtunnel.SSHTunnelForwarder(
        remote_bind_address=(remote_host, int(remote_port)),
        local_bind_address=("127.0.0.1", 0),
        logger=get_quiet_sshtunnel_logger(),
        **create_ssh_connect_kwargs(config["vps"]),
    )
    tunnel.start()
    return tunnel


def open_db_connection(config: dict):
    pymysql = load_optional_dependency("pymysql", "PyMySQL")

    database = config["database"]
    tunnel = None
    host = database["host"]
    port = int(database.get("port", 3306))

    if database.get("use_ssh_tunnel", True):
        tunnel_config = database.get("ssh_tunnel", {})
        remote_host = tunnel_config.get("remote_host", "127.0.0.1")
        remote_port = int(tunnel_config.get("remote_port", port))

        tunnel = start_ssh_tunnel(config, remote_host, remote_port)
        host = "127.0.0.1"
        port = int(tunnel.local_bind_port)

    try:
        connection = pymysql.connect(
            host=host,
            port=port,
            user=database["user"],
            password=database["password"],
            database=database["name"],
            charset="utf8mb4",
            autocommit=False,
            cursorclass=pymysql.cursors.DictCursor,
        )
        return connection, tunnel
    except pymysql.err.OperationalError as exc:
        if not database.get("use_ssh_tunnel", True):
            raise

        with contextlib.suppress(Exception):
            if tunnel is not None:
                tunnel.stop()

        tunnel_config = database.get("ssh_tunnel", {})
        remote_host = tunnel_config.get("remote_host", "127.0.0.1")
        fallback_host = discover_mysql_container_host(config)
        if not fallback_host or fallback_host == remote_host:
            raise RuntimeError(
                "No se pudo conectar a MySQL ni por tunel a host local del VPS ni resolviendo el contenedor Docker. "
                f"Detalle original: {exc}"
            ) from exc

        tunnel = start_ssh_tunnel(config, fallback_host, int(tunnel_config.get("remote_port", database.get("port", 3306))))
        connection = pymysql.connect(
            host="127.0.0.1",
            port=int(tunnel.local_bind_port),
            user=database["user"],
            password=database["password"],
            database=database["name"],
            charset="utf8mb4",
            autocommit=False,
            cursorclass=pymysql.cursors.DictCursor,
        )
        return connection, tunnel


def open_sftp_client(config: dict):
    ssh_client = open_ssh_client(config)
    return ssh_client, ssh_client.open_sftp()


def ensure_remote_directory(sftp, remote_directory: str):
    normalized = posixpath.normpath(remote_directory)
    current = "/" if normalized.startswith("/") else ""
    for part in normalized.split("/"):
        if not part:
            continue
        current = posixpath.join(current, part) if current else part
        try:
            sftp.stat(current)
        except IOError:
            sftp.mkdir(current)


def build_snapshot_base_path_candidates(config: dict) -> list[str]:
    configured = config["snapshots"]["base_path"].rstrip("/")
    fallback = f"/home/{config['vps']['username']}/vehicle_surveillance/snapshots"
    candidates = [configured]
    if fallback != configured:
        candidates.append(fallback)
    return candidates


def derive_source_type(source_path: str) -> str:
    source = source_path.strip()
    if source.isdigit():
        return "camera"
    if source.lower().startswith("rtsp://"):
        return "rtsp"
    suffix = Path(source).suffix.lower().lstrip(".")
    return suffix or "file"


def get_or_create_video_source(cursor, payload: dict):
    source_path = payload["source_path"]
    cursor.execute(
        "SELECT id FROM video_sources WHERE source_path = %s LIMIT 1",
        (source_path,),
    )
    existing = cursor.fetchone()
    if existing:
        return int(existing["id"])

    source_type = payload.get("source_type") or derive_source_type(source_path)
    source_name = payload.get("source_name") or infer_source_name(source_path)
    cursor.execute(
        """
        INSERT INTO video_sources (name, source_type, source_path, description, location, is_active)
        VALUES (%s, %s, %s, %s, %s, %s)
        """,
        (
            source_name,
            source_type,
            source_path,
            payload.get("description"),
            payload.get("location"),
            1,
        ),
    )
    return int(cursor.lastrowid)


def start_processing_session(connection, config: dict, payload: dict):
    application_config = config.get("application", {})
    with connection.cursor() as cursor:
        video_source_id = get_or_create_video_source(cursor, payload)
        cursor.execute(
            """
            INSERT INTO processing_sessions (
                video_source_id,
                started_at,
                status,
                yolo_model,
                tracking_model,
                frame_skip,
                confidence_threshold,
                iou_threshold,
                notes,
                created_by_user_id
            )
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
            """,
            (
                video_source_id,
                normalize_timestamp(payload.get("started_at_utc")),
                payload.get("status", "running"),
                payload.get("yolo_model") or application_config.get("default_yolo_model", "yolo11n.pt"),
                payload.get("tracking_model") or application_config.get("tracking_model", "manual_capture_no_tracker"),
                int(payload.get("frame_skip", application_config.get("frame_skip", 0))),
                float(payload.get("confidence_threshold", application_config.get("confidence_threshold", 0.35))),
                payload.get("iou_threshold", application_config.get("iou_threshold")),
                payload.get("notes") or application_config.get("notes"),
                payload.get("created_by_user_id", application_config.get("created_by_user_id")),
            ),
        )
        processing_session_id = int(cursor.lastrowid)
    connection.commit()
    return {
        "video_source_id": video_source_id,
        "processing_session_id": processing_session_id,
    }


def resolve_external_track_id(snapshot: dict) -> int:
    track_id = parse_optional_nonnegative_int(snapshot.get("track_id"))
    if track_id is not None:
        return fit_external_track_id(track_id)

    return build_synthetic_external_track_id(snapshot)


def upsert_track(cursor, processing_session_id: int, snapshot: dict):
    track_id = parse_optional_nonnegative_int(snapshot.get("track_id"))
    external_track_id = resolve_external_track_id(snapshot)
    vehicle_class = snapshot.get("label") or str(snapshot.get("class_id", "vehicle"))
    captured_at = normalize_timestamp(snapshot.get("captured_at_utc"))
    frame_index = int(snapshot["frame_id"])
    confidence = float(snapshot["confidence"])
    status = "active" if track_id is not None else "manual_capture"

    cursor.execute(
        """
        SELECT id, frame_count, avg_confidence, max_confidence
        FROM vehicle_tracks
        WHERE processing_session_id = %s AND external_track_id = %s
        LIMIT 1
        """,
        (processing_session_id, external_track_id),
    )
    existing = cursor.fetchone()
    if existing:
        frame_count = int(existing.get("frame_count") or 0)
        avg_confidence = float(existing.get("avg_confidence") or 0.0)
        max_confidence = float(existing.get("max_confidence") or 0.0)
        new_frame_count = frame_count + 1
        new_avg = ((avg_confidence * frame_count) + confidence) / new_frame_count if new_frame_count else confidence
        new_max = max(max_confidence, confidence)
        cursor.execute(
            """
            UPDATE vehicle_tracks
            SET ended_at = %s,
                end_frame_index = %s,
                frame_count = %s,
                avg_confidence = %s,
                max_confidence = %s,
                status = %s,
                updated_at = CURRENT_TIMESTAMP
            WHERE id = %s
            """,
            (
                captured_at,
                frame_index,
                new_frame_count,
                new_avg,
                new_max,
                status,
                int(existing["id"]),
            ),
        )
        return int(existing["id"]), external_track_id

    cursor.execute(
        """
        INSERT INTO vehicle_tracks (
            processing_session_id,
            external_track_id,
            vehicle_class,
            started_at,
            ended_at,
            start_frame_index,
            end_frame_index,
            frame_count,
            avg_confidence,
            max_confidence,
            status
        )
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """,
        (
            processing_session_id,
            external_track_id,
            vehicle_class,
            captured_at,
            captured_at,
            frame_index,
            frame_index,
            1,
            confidence,
            confidence,
            status,
        ),
    )
    return int(cursor.lastrowid), external_track_id


def insert_detection(cursor, track_id: int, snapshot: dict):
    bbox = snapshot["bbox"]
    x1, y1, x2, y2 = [int(value) for value in bbox]
    width = max(0, x2 - x1)
    height = max(0, y2 - y1)
    center_x = x1 + width / 2.0
    center_y = y1 + height / 2.0

    cursor.execute(
        """
        INSERT INTO vehicle_detections (
            track_id,
            detected_at,
            frame_index,
            bbox_x1,
            bbox_y1,
            bbox_x2,
            bbox_y2,
            confidence,
            vehicle_class,
            center_x,
            center_y,
            width,
            height
        )
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """,
        (
            track_id,
            normalize_timestamp(snapshot.get("captured_at_utc")),
            int(snapshot["frame_id"]),
            x1,
            y1,
            x2,
            y2,
            float(snapshot["confidence"]),
            snapshot.get("label") or str(snapshot.get("class_id", "vehicle")),
            center_x,
            center_y,
            width,
            height,
        ),
    )
    return int(cursor.lastrowid)


def upload_snapshot_file(config: dict, processing_session_id: int, track_id: int, snapshot: dict):
    local_image_path = snapshot["local_image_path"]
    if not os.path.exists(local_image_path):
        raise RuntimeError(f"No existe la imagen local a subir: {local_image_path}")

    ssh_client, sftp = open_sftp_client(config)
    try:
        last_error = None
        for base_path in build_snapshot_base_path_candidates(config):
            remote_directory = posixpath.join(
                base_path,
                f"session_{processing_session_id}",
                f"track_{track_id}",
            )
            remote_file_path = posixpath.join(remote_directory, f"{snapshot['snapshot_id']}.jpg")
            try:
                ensure_remote_directory(sftp, remote_directory)
                sftp.put(local_image_path, remote_file_path)
                return remote_file_path, int(os.path.getsize(local_image_path))
            except (PermissionError, OSError, IOError) as exc:
                last_error = exc

        if last_error is not None:
            raise RuntimeError(
                "No se pudo subir la captura al VPS por permisos o ruta remota invalida. "
                f"Detalle: {last_error}"
            ) from last_error
    finally:
        try:
            sftp.close()
        finally:
            ssh_client.close()

    raise RuntimeError("No se pudo subir la captura al VPS por un motivo desconocido.")


def upload_related_file(
    config: dict,
    processing_session_id: int,
    track_id: int,
    local_file_path: str,
    remote_file_name: str,
):
    if not os.path.exists(local_file_path):
        raise RuntimeError(f"No existe el archivo local a subir: {local_file_path}")

    ssh_client, sftp = open_sftp_client(config)
    try:
        last_error = None
        for base_path in build_snapshot_base_path_candidates(config):
            remote_directory = posixpath.join(
                base_path,
                f"session_{processing_session_id}",
                f"track_{track_id}",
            )
            remote_file_path = posixpath.join(remote_directory, remote_file_name)
            try:
                ensure_remote_directory(sftp, remote_directory)
                sftp.put(local_file_path, remote_file_path)
                return remote_file_path, int(os.path.getsize(local_file_path))
            except (PermissionError, OSError, IOError) as exc:
                last_error = exc

        if last_error is not None:
            raise RuntimeError(
                "No se pudo subir el archivo relacionado al VPS por permisos o ruta remota invalida. "
                f"Detalle: {last_error}"
            ) from last_error
    finally:
        try:
            sftp.close()
        finally:
            ssh_client.close()

    raise RuntimeError("No se pudo subir el archivo relacionado al VPS por un motivo desconocido.")


def upload_analysis_artifact(config: dict, payload: dict):
    processing_session_id = int(payload["processing_session_id"])
    track_id = int(payload["track_id"])
    snapshot_id = str(payload["snapshot_id"])
    local_analysis_path = payload["local_analysis_path"]

    remote_analysis_path, file_size_bytes = upload_related_file(
        config,
        processing_session_id,
        track_id,
        local_analysis_path,
        f"{snapshot_id}.analysis.json",
    )

    return {
        "processing_session_id": processing_session_id,
        "track_id": track_id,
        "snapshot_id": snapshot_id,
        "remote_analysis_path": remote_analysis_path,
        "file_size_bytes": file_size_bytes,
    }


def insert_snapshot(cursor, track_id: int, detection_id: int, snapshot: dict, remote_file_path: str, file_size_bytes: int):
    cursor.execute(
        """
        INSERT INTO vehicle_snapshots (
            track_id,
            detection_id,
            file_path,
            snapshot_type,
            captured_at,
            frame_index,
            width,
            height,
            file_size_bytes,
            is_best
        )
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        """,
        (
            track_id,
            detection_id,
            remote_file_path,
            snapshot.get("snapshot_type", "manual"),
            normalize_timestamp(snapshot.get("captured_at_utc")),
            int(snapshot["frame_id"]),
            int(snapshot["crop_width"]),
            int(snapshot["crop_height"]),
            file_size_bytes,
            1 if snapshot.get("is_best") else 0,
        ),
    )
    snapshot_id = int(cursor.lastrowid)
    if snapshot.get("is_best"):
        cursor.execute(
            "UPDATE vehicle_tracks SET best_snapshot_id = %s, updated_at = CURRENT_TIMESTAMP WHERE id = %s",
            (snapshot_id, track_id),
        )
    return snapshot_id


def persist_snapshot(connection, config: dict, payload: dict):
    processing_session_id = int(payload["processing_session_id"])
    snapshot = payload["snapshot"]

    with connection.cursor() as cursor:
        cursor.execute(
            "SELECT id FROM processing_sessions WHERE id = %s LIMIT 1",
            (processing_session_id,),
        )
        processing_session = cursor.fetchone()
        if not processing_session:
            raise RuntimeError(
                f"No existe processing_sessions.id={processing_session_id} para persistir el snapshot."
            )

        track_id, external_track_id = upsert_track(cursor, processing_session_id, snapshot)
        detection_id = insert_detection(cursor, track_id, snapshot)
        remote_file_path, file_size_bytes = upload_snapshot_file(
            config,
            processing_session_id,
            track_id,
            snapshot,
        )
        db_snapshot_id = insert_snapshot(
            cursor,
            track_id,
            detection_id,
            snapshot,
            remote_file_path,
            file_size_bytes,
        )

    connection.commit()
    return {
        "processing_session_id": processing_session_id,
        "track_id": track_id,
        "external_track_id": external_track_id,
        "detection_id": detection_id,
        "snapshot_id": db_snapshot_id,
        "remote_file_path": remote_file_path,
    }


def finish_processing_session(connection, payload: dict):
    processing_session_id = int(payload["processing_session_id"])
    ended_at = normalize_timestamp(payload.get("ended_at_utc"))
    status = payload.get("status", "completed")

    with connection.cursor() as cursor:
        cursor.execute(
            """
            UPDATE vehicle_tracks
            SET ended_at = COALESCE(ended_at, %s),
                status = CASE WHEN status = 'active' THEN %s ELSE status END,
                updated_at = CURRENT_TIMESTAMP
            WHERE processing_session_id = %s
            """,
            (ended_at, status, processing_session_id),
        )
        cursor.execute(
            "UPDATE processing_sessions SET ended_at = %s, status = %s WHERE id = %s",
            (ended_at, status, processing_session_id),
        )

    connection.commit()
    return {"processing_session_id": processing_session_id}


def list_history(connection, payload: dict):
    """Lista las capturas (vehicle_snapshots) para el historial.

    Une snapshots -> tracks -> processing_sessions y filtra por created_by_user_id
    cuando se provee user_id, de modo que cada usuario ve solo sus capturas.
    """
    user_id = parse_optional_nonnegative_int(payload.get("user_id"))
    per_page = max(1, int(payload.get("per_page") or 20))
    page = max(1, int(payload.get("page") or 1))
    offset = (page - 1) * per_page

    where = ""
    filter_params: list = []
    if user_id is not None:
        where = "WHERE ps.created_by_user_id = %s"
        filter_params.append(user_id)

    with connection.cursor() as cursor:
        cursor.execute(
            f"""
            SELECT COUNT(*) AS total
            FROM vehicle_snapshots s
            JOIN vehicle_tracks t ON s.track_id = t.id
            JOIN processing_sessions ps ON t.processing_session_id = ps.id
            {where}
            """,
            filter_params,
        )
        total = int(cursor.fetchone()["total"])

        cursor.execute(
            f"""
            SELECT s.id AS id,
                   ps.id AS session_id,
                   s.file_path AS snapshot_path,
                   s.captured_at AS captured_at,
                   COALESCE(d.vehicle_class, t.vehicle_class) AS vehicle_class,
                   COALESCE(d.confidence, t.max_confidence) AS confidence,
                   a.id AS analysis_id,
                   a.attributes_json AS attributes_json
            FROM vehicle_snapshots s
            JOIN vehicle_tracks t ON s.track_id = t.id
            JOIN processing_sessions ps ON t.processing_session_id = ps.id
            LEFT JOIN vehicle_detections d ON s.detection_id = d.id
            LEFT JOIN vehicle_ai_analysis a ON a.snapshot_id = s.id
            {where}
            ORDER BY s.captured_at DESC, s.id DESC
            LIMIT %s OFFSET %s
            """,
            filter_params + [per_page, offset],
        )
        rows = cursor.fetchall()

    vehicles = []
    for row in rows:
        attrs = {}
        raw_attrs = row.get("attributes_json")
        if raw_attrs:
            try:
                attrs = raw_attrs if isinstance(raw_attrs, dict) else json.loads(raw_attrs)
            except (ValueError, TypeError):
                attrs = {}

        # captured_at se guarda como UTC naive; lo pasamos a hora de Argentina.
        captured = row.get("captured_at")
        if hasattr(captured, "strftime"):
            detected_at = (captured + ARGENTINA_UTC_OFFSET).strftime("%Y-%m-%dT%H:%M:%S")
        else:
            detected_at = str(captured) if captured else ""

        vehicles.append(
            {
                "id": int(row["id"]),
                "session_id": int(row["session_id"]) if row.get("session_id") is not None else 0,
                "snapshot_path": row.get("snapshot_path") or "",
                "snapshot_url": "",
                "vehicle_type": (row.get("vehicle_class") or "").lower(),
                "brand": attrs.get("make") or attrs.get("brand") or "",
                "model": attrs.get("model") or "",
                "year_range": attrs.get("year_range") or "",
                "color": attrs.get("color") or "",
                "license_plate": attrs.get("license_plate") or attrs.get("plate") or "",
                "confidence": float(row["confidence"]) if row.get("confidence") is not None else 0.0,
                "detected_at": detected_at,
                "has_analysis": bool(row.get("analysis_id")),
            }
        )

    return {"vehicles": vehicles, "total": total}


def save_analysis(connection, payload: dict):
    """Inserta el resultado del analisis IA en vehicle_ai_analysis.

    Asi el historial puede mostrar marca/modelo/color y has_analysis=1.
    """
    track_id = parse_optional_nonnegative_int(payload.get("track_id"))
    snapshot_id = parse_optional_nonnegative_int(payload.get("snapshot_id"))
    if track_id is None or snapshot_id is None:
        raise RuntimeError("save-analysis requiere track_id y snapshot_id validos.")

    requested_by = parse_optional_nonnegative_int(payload.get("requested_by_user_id"))
    model_name = payload.get("model_name") or "gemini"
    prompt_version = payload.get("prompt_version") or "v1"  # columna NOT NULL
    summary_text = payload.get("summary_text") or ""
    attributes = payload.get("attributes") or {}
    attributes_json = json.dumps(attributes, ensure_ascii=False)
    raw_response = payload.get("raw_response")
    raw_response_json = json.dumps(raw_response, ensure_ascii=False) if raw_response is not None else None

    with connection.cursor() as cursor:
        cursor.execute(
            """
            INSERT INTO vehicle_ai_analysis (
                track_id, snapshot_id, requested_by_user_id, model_name,
                prompt_version, summary_text, attributes_json, raw_response
            )
            VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
            """,
            (
                track_id,
                snapshot_id,
                requested_by,
                model_name,
                prompt_version,
                summary_text,
                attributes_json,
                raw_response_json,
            ),
        )
        analysis_id = int(cursor.lastrowid)
    connection.commit()
    return {"analysis_id": analysis_id, "track_id": track_id, "snapshot_id": snapshot_id}


def get_analysis(connection, payload: dict):
    """Devuelve el detalle de una captura + su analisis IA (si existe).

    Salida: { "vehicle": {...}, "analysis": {...} } para la pantalla de detalle.
    """
    snapshot_id = parse_optional_nonnegative_int(
        payload.get("vehicle_id") if payload.get("vehicle_id") is not None else payload.get("snapshot_id")
    )
    if snapshot_id is None:
        raise RuntimeError("get-analysis requiere vehicle_id (id de la captura).")

    with connection.cursor() as cursor:
        cursor.execute(
            """
            SELECT s.id AS id,
                   ps.id AS session_id,
                   s.file_path AS snapshot_path,
                   s.captured_at AS captured_at,
                   COALESCE(d.vehicle_class, t.vehicle_class) AS vehicle_class,
                   COALESCE(d.confidence, t.max_confidence) AS confidence,
                   a.id AS analysis_id,
                   a.summary_text AS summary_text,
                   a.attributes_json AS attributes_json,
                   a.model_name AS model_name,
                   a.created_at AS analyzed_at
            FROM vehicle_snapshots s
            JOIN vehicle_tracks t ON s.track_id = t.id
            JOIN processing_sessions ps ON t.processing_session_id = ps.id
            LEFT JOIN vehicle_detections d ON s.detection_id = d.id
            LEFT JOIN vehicle_ai_analysis a ON a.snapshot_id = s.id
            WHERE s.id = %s
            ORDER BY a.id DESC
            LIMIT 1
            """,
            (snapshot_id,),
        )
        row = cursor.fetchone()

    if not row:
        raise RuntimeError(f"No existe la captura id={snapshot_id}.")

    attrs = {}
    raw_attrs = row.get("attributes_json")
    if raw_attrs:
        try:
            attrs = raw_attrs if isinstance(raw_attrs, dict) else json.loads(raw_attrs)
        except (ValueError, TypeError):
            attrs = {}

    captured = row.get("captured_at")
    detected_at = (captured + ARGENTINA_UTC_OFFSET).strftime("%Y-%m-%dT%H:%M:%S") if hasattr(captured, "strftime") else ""
    analyzed = row.get("analyzed_at")
    analyzed_at = (analyzed + ARGENTINA_UTC_OFFSET).strftime("%Y-%m-%dT%H:%M:%S") if hasattr(analyzed, "strftime") else ""

    make = attrs.get("make") or attrs.get("brand") or ""
    model = attrs.get("model") or ""
    color = attrs.get("color") or ""
    body_type = attrs.get("body_type") or ""
    year_range = attrs.get("year_range") or ""
    confidence_ai = attrs.get("confidence")
    summary = row.get("summary_text") or ""
    has_analysis = bool(row.get("analysis_id"))

    vehicle = {
        "id": int(row["id"]),
        "session_id": int(row["session_id"]) if row.get("session_id") is not None else 0,
        "snapshot_path": row.get("snapshot_path") or "",
        "snapshot_url": "",
        "vehicle_type": (row.get("vehicle_class") or "").lower(),
        "brand": make,
        "model": model,
        "year_range": year_range,
        "color": color,
        "license_plate": "",
        "confidence": float(row["confidence"]) if row.get("confidence") is not None else 0.0,
        "detected_at": detected_at,
        "has_analysis": has_analysis,
    }

    full_lines = []
    if has_analysis:
        if make:
            full_lines.append(f"Marca: {make}")
        if model:
            full_lines.append(f"Modelo: {model}")
        if color:
            full_lines.append(f"Color: {color}")
        if body_type:
            full_lines.append(f"Carrocería: {body_type}")
        if year_range:
            full_lines.append(f"Año aprox.: {year_range}")
        if confidence_ai is not None:
            full_lines.append(f"Confianza IA: {confidence_ai}")
        if row.get("model_name"):
            full_lines.append(f"Modelo IA: {row.get('model_name')}")
        if summary:
            full_lines.append("")
            full_lines.append(summary)
    else:
        full_lines.append("Esta captura todavía no tiene análisis de IA.")

    analysis = {
        "id": int(row["analysis_id"]) if row.get("analysis_id") is not None else 0,
        "vehicle_id": int(row["id"]),
        "brand": make,
        "model": model,
        "color": color,
        "vehicle_type": vehicle["vehicle_type"],
        "license_plate": "",
        "observations": summary,
        "full_text": "\n".join(full_lines),
        "analyzed_at": analyzed_at,
    }

    return {"vehicle": vehicle, "analysis": analysis}


def build_parser():
    parser = argparse.ArgumentParser(description="Persistencia remota de snapshots de vehiculos")
    parser.add_argument(
        "command",
        choices=[
            "start-session",
            "persist-snapshot",
            "finish-session",
            "upload-analysis",
            "list-history",
            "save-analysis",
            "get-analysis",
        ],
    )
    parser.add_argument("--config", required=True, help="Ruta al archivo JSON de configuracion remota")
    parser.add_argument("--payload", required=True, help="Ruta al archivo JSON con el payload de la operacion")
    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    try:
        config = load_json_file(args.config)
        require_config_fields(config)
        payload = load_json_file(args.payload)

        if args.command == "upload-analysis":
            data = upload_analysis_artifact(config, payload)
            print(json.dumps({"success": True, "data": data}, ensure_ascii=True))
            return

        connection, tunnel = open_db_connection(config)
        try:
            if args.command == "start-session":
                data = start_processing_session(connection, config, payload)
            elif args.command == "persist-snapshot":
                data = persist_snapshot(connection, config, payload)
            elif args.command == "list-history":
                data = list_history(connection, payload)
            elif args.command == "save-analysis":
                data = save_analysis(connection, payload)
            elif args.command == "get-analysis":
                data = get_analysis(connection, payload)
            else:
                data = finish_processing_session(connection, payload)
        finally:
            with contextlib.suppress(Exception):
                connection.close()
            if tunnel is not None:
                with contextlib.suppress(Exception):
                    tunnel.stop()

        print(json.dumps({"success": True, "data": data}, ensure_ascii=True))
    except Exception as exc:
        print(json.dumps({"success": False, "error": str(exc)}, ensure_ascii=True), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()