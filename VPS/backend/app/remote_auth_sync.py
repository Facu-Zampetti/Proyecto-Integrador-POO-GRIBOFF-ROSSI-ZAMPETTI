"""Helper de autenticacion contra la MySQL del VPS (users / user_sessions).

Reutiliza los helpers de conexion de remote_snapshot_sync (tunel SSH + pymysql)
porque la base vehicle_surveillance solo es alcanzable por el tunel SSH ya probado.

Comandos:
  register-user  payload: {username, email, password_hash, full_name?, role?}
  login-user     payload: {username, password_hash, ip_address?, user_agent?,
                           session_ttl_hours?}

Salida JSON: {"success": bool, "data"/"error": ...}, igual que el helper de snapshots.
No modifica el esquema (sin ALTER); solo INSERT/SELECT/UPDATE sobre tablas existentes.
"""

import argparse
import contextlib
import json
import secrets
import sys
from datetime import datetime, timedelta, timezone

import pymysql.err

import remote_snapshot_sync as base

DEFAULT_SESSION_TTL_HOURS = 24


def _now_utc() -> datetime:
    return datetime.now(timezone.utc)


def _to_mysql_dt(dt: datetime) -> str:
    return base.to_mysql_datetime_string(dt)


def register_user(connection, payload: dict) -> dict:
    username = str(payload.get("username", "")).strip()
    email = str(payload.get("email", "")).strip()
    password_hash = str(payload.get("password_hash", "")).strip()
    if not username or not email or not password_hash:
        raise RuntimeError("Faltan campos obligatorios: username, email y password_hash.")

    full_name = payload.get("full_name") or username

    with connection.cursor() as cursor:
        try:
            if payload.get("role"):
                cursor.execute(
                    """
                    INSERT INTO users (username, email, password_hash, full_name, role)
                    VALUES (%s, %s, %s, %s, %s)
                    """,
                    (username, email, password_hash, full_name, str(payload["role"])),
                )
            else:
                # Deja que el esquema aplique el default de role (operator) e is_active (1).
                cursor.execute(
                    """
                    INSERT INTO users (username, email, password_hash, full_name)
                    VALUES (%s, %s, %s, %s)
                    """,
                    (username, email, password_hash, full_name),
                )
        except pymysql.err.IntegrityError as exc:
            # 1062 = Duplicate entry (username o email UNIQUE)
            connection.rollback()
            detail = str(exc).lower()
            if "email" in detail:
                raise RuntimeError("El email ya esta registrado.") from exc
            raise RuntimeError("El nombre de usuario ya esta registrado.") from exc

        user_id = int(cursor.lastrowid)
        cursor.execute(
            "SELECT id, username, email, role, is_active FROM users WHERE id = %s",
            (user_id,),
        )
        row = cursor.fetchone()

    connection.commit()
    return {
        "id": int(row["id"]),
        "username": row["username"],
        "email": row["email"],
        "role": row["role"],
        "is_active": int(row["is_active"]),
    }


def login_user(connection, payload: dict) -> dict:
    username = str(payload.get("username", "")).strip()
    password_hash = str(payload.get("password_hash", "")).strip()
    if not username or not password_hash:
        raise RuntimeError("Faltan campos obligatorios: username y password_hash.")

    ttl_hours = int(payload.get("session_ttl_hours") or DEFAULT_SESSION_TTL_HOURS)

    with connection.cursor() as cursor:
        cursor.execute(
            "SELECT id, username, email, password_hash, role, is_active FROM users WHERE username = %s LIMIT 1",
            (username,),
        )
        user = cursor.fetchone()
        if not user:
            raise RuntimeError("Usuario o contrasena incorrectos.")
        if not int(user["is_active"]):
            raise RuntimeError("La cuenta esta desactivada.")
        if not secrets.compare_digest(str(user["password_hash"]), password_hash):
            raise RuntimeError("Usuario o contrasena incorrectos.")

        user_id = int(user["id"])
        now = _now_utc()
        expires_at = now + timedelta(hours=ttl_hours)
        session_token = secrets.token_hex(32)

        cursor.execute(
            "UPDATE users SET last_login_at = %s WHERE id = %s",
            (_to_mysql_dt(now), user_id),
        )
        cursor.execute(
            """
            INSERT INTO user_sessions (user_id, session_token, expires_at, ip_address, user_agent)
            VALUES (%s, %s, %s, %s, %s)
            """,
            (
                user_id,
                session_token,
                _to_mysql_dt(expires_at),
                payload.get("ip_address"),
                payload.get("user_agent"),
            ),
        )

    connection.commit()
    return {
        "id": user_id,
        "username": user["username"],
        "email": user["email"],
        "role": user["role"],
        "session_token": session_token,
        "expires_at": _to_mysql_dt(expires_at),
    }


def build_parser():
    parser = argparse.ArgumentParser(description="Autenticacion remota contra MySQL del VPS")
    parser.add_argument("command", choices=["register-user", "login-user"])
    parser.add_argument("--config", required=True, help="Ruta al JSON de configuracion remota")
    parser.add_argument("--payload", required=True, help="Ruta al JSON con el payload de la operacion")
    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    try:
        config = base.load_json_file(args.config)
        base.require_config_fields(config)
        payload = base.load_json_file(args.payload)

        connection, tunnel = base.open_db_connection(config)
        try:
            if args.command == "register-user":
                data = register_user(connection, payload)
            else:
                data = login_user(connection, payload)
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
