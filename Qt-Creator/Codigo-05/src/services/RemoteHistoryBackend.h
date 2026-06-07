#ifndef REMOTEHISTORYBACKEND_H
#define REMOTEHISTORYBACKEND_H

#include <QList>
#include <QString>

#include "models/VehicleModel.h"
#include "models/AnalysisModel.h"

/**
 * @file RemoteHistoryBackend.h
 * @brief Puente C++ -> helper Python (backend/remote_snapshot_sync.py list-history)
 *        para leer el historial de capturas desde la MySQL del VPS via tunel SSH.
 *
 * No existe backend REST; este es el unico camino funcional a la base remota,
 * igual que RemoteAuthBackend. La operacion es SINCRONA (abre tunel SSH): el
 * llamador debe ejecutarla en un hilo de fondo para no congelar la UI.
 */
class RemoteHistoryBackend {
public:
    struct Result {
        bool                success = false;
        QString             error;
        QList<VehicleModel> vehicles;
        int                 total = 0;
    };

    struct AnalysisDetail {
        bool          success = false;
        QString       error;
        VehicleModel  vehicle;
        AnalysisModel analysis;
    };

    RemoteHistoryBackend() = default;

    /// Lista las capturas del usuario (filtra por created_by_user_id si userId>0).
    Result listHistory(int userId, int page, int perPage);

    /// Detalle de una captura + su analisis IA (para la pantalla "Ver Análisis").
    AnalysisDetail getAnalysis(int vehicleId);

private:
    /// Ejecuta el helper Python (list-history / get-analysis) y devuelve el
    /// objeto "data" en caso de exito. Reusa la resolucion de rutas y QProcess.
    bool runHelper(const QString& command, const QJsonObject& payload,
                   QJsonObject* dataOut, QString* errorOut, QString* projectRootOut);

    QString findProjectRoot() const;
    QString resolvePythonExe(const QString& projectRoot) const;
    QString resolveHelperPath(const QString& projectRoot) const;
    QString resolveConfigPath(const QString& projectRoot) const;
};

#endif // REMOTEHISTORYBACKEND_H
