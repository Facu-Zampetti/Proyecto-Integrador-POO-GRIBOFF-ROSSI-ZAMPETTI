#ifndef APPENUMS_H
#define APPENUMS_H

/**
 * @file AppEnums.h
 * @brief Enumeraciones globales de la aplicación Carlens.
 *
 * Centraliza todos los tipos enumerados del sistema para garantizar
 * consistencia en toda la arquitectura (encapsulamiento de dominio).
 */

namespace Carlens {

// ─── Pantallas de la aplicación ───────────────────────────────────────────
enum class AppScreen {
    Login,
    Register,
    Dashboard
};

// ─── Estado de una petición HTTP ──────────────────────────────────────────
enum class ApiStatus {
    Idle,
    Loading,
    Success,
    Error
};

// ─── Tipo de vehículo detectado por YOLO ──────────────────────────────────
enum class VehicleType {
    Car,
    Truck,
    Bus,
    Motorcycle,
    Van,
    Bicycle,
    Unknown
};

// ─── Estado del stream RTSP ───────────────────────────────────────────────
enum class StreamStatus {
    Disconnected,
    Connecting,
    Connected,
    Error
};

// ─── Sección activa en el Dashboard ───────────────────────────────────────
enum class DashboardSection {
    VideoUpload,
    Rtsp,
    History,
    Analysis,
    Settings
};

// ─── Estado de carga de un video ─────────────────────────────────────────
enum class UploadStatus {
    Idle,
    Uploading,
    Processing,
    Completed,
    Failed
};

} // namespace Carlens

#endif // APPENUMS_H
