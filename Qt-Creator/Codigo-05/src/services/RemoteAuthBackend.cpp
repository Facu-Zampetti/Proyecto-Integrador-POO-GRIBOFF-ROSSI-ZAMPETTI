#include "services/RemoteAuthBackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QTemporaryFile>

namespace {
constexpr const char* kHelperRelPath = "backend/remote_auth_sync.py";
constexpr const char* kConfigRelPath = "config/vehicle_surveillance.remote.json";
constexpr const char* kIaSubdir      = "Codigo_Implementacion_IA";
}

RemoteAuthBackend::Result RemoteAuthBackend::registerUser(const QString& username,
                                                          const QString& email,
                                                          const QString& passwordHash)
{
    QJsonObject payload;
    payload.insert("username", username);
    payload.insert("email", email);
    payload.insert("password_hash", passwordHash);
    return executeHelper(QStringLiteral("register-user"), payload);
}

RemoteAuthBackend::Result RemoteAuthBackend::login(const QString& username,
                                                   const QString& passwordHash,
                                                   const QString& ipAddress,
                                                   const QString& userAgent)
{
    QJsonObject payload;
    payload.insert("username", username);
    payload.insert("password_hash", passwordHash);
    if (!ipAddress.isEmpty())
        payload.insert("ip_address", ipAddress);
    if (!userAgent.isEmpty())
        payload.insert("user_agent", userAgent);
    return executeHelper(QStringLiteral("login-user"), payload);
}

RemoteAuthBackend::Result RemoteAuthBackend::executeHelper(const QString& command,
                                                           const QJsonObject& payload)
{
    Result result;

    const QString projectRoot = findProjectRoot();
    const QString pythonExe = resolvePythonExe(projectRoot);
    const QString helperPath = resolveHelperPath(projectRoot);
    const QString configPath = resolveConfigPath(projectRoot);

    if (helperPath.isEmpty()) {
        result.error = QStringLiteral(
            "No se encontro el helper de autenticacion (backend/remote_auth_sync.py). "
            "Verifique la instalacion del proyecto.");
        return result;
    }
    if (!QFileInfo::exists(configPath)) {
        result.error = QStringLiteral(
            "Falta config/vehicle_surveillance.remote.json; no se puede contactar la base de datos.");
        return result;
    }

    QTemporaryFile payloadFile;
    payloadFile.setAutoRemove(true);
    if (!payloadFile.open()) {
        result.error = QStringLiteral("No se pudo crear un archivo temporal para la peticion de autenticacion.");
        return result;
    }
    payloadFile.write(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    payloadFile.flush();

    QProcess process;
    process.setProgram(pythonExe);
    process.setArguments({helperPath,
                          command,
                          QStringLiteral("--config"), configPath,
                          QStringLiteral("--payload"), payloadFile.fileName()});
    process.start();

    if (!process.waitForStarted(5000)) {
        result.error = QStringLiteral("No se pudo iniciar el entorno Python de autenticacion.");
        return result;
    }

    // El tunel SSH + conexion MySQL puede tardar varios segundos.
    if (!process.waitForFinished(45000)) {
        process.kill();
        process.waitForFinished(2000);
        result.error = QStringLiteral("La autenticacion contra el servidor no respondio a tiempo.");
        return result;
    }

    const QByteArray stdOut = process.readAllStandardOutput().trimmed();
    const QByteArray stdErr = process.readAllStandardError().trimmed();

    // El helper imprime su JSON de error por stderr cuando falla; intentamos parsearlo.
    const QByteArray jsonBytes = !stdOut.isEmpty() ? stdOut : stdErr;
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = stdErr.isEmpty()
            ? QStringLiteral("La respuesta del servidor de autenticacion no es valida.")
            : QString::fromUtf8(stdErr);
        return result;
    }

    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("success")).toBool()) {
        result.error = obj.value(QStringLiteral("error")).toString(
            QStringLiteral("La operacion de autenticacion fallo sin detalle."));
        return result;
    }

    result.success = true;
    result.data = obj.value(QStringLiteral("data")).toObject();
    return result;
}

// Devuelve la raiz que contiene backend/remote_auth_sync.py a partir de un dir
// base: el propio base, o base/Codigo_Implementacion_IA. Cadena vacia si nada.
static QString matchAuthRoot(const QDir& base)
{
    const QString here = base.filePath(QString::fromLatin1(kHelperRelPath));
    if (QFileInfo::exists(here))
        return base.absolutePath();

    const QString nested = base.filePath(
        QString::fromLatin1(kIaSubdir) + QLatin1Char('/') + QString::fromLatin1(kHelperRelPath));
    if (QFileInfo::exists(nested))
        return QDir(base.filePath(QString::fromLatin1(kIaSubdir))).absolutePath();

    return QString();
}

QString RemoteAuthBackend::findProjectRoot() const
{
    QDir dir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 8; ++i) {
        // 1) El propio ancestro (o su subdir Codigo_Implementacion_IA).
        const QString direct = matchAuthRoot(dir);
        if (!direct.isEmpty())
            return direct;

        // 2) Un nivel hacia abajo: cubre el shadow-build de QtCreator, que queda
        //    como carpeta hermana del proyecto (p.ej. C:/Carlens/build-... con
        //    el proyecto en C:/Carlens/Codigo-03/Codigo_Implementacion_IA).
        const QFileInfoList children =
            dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& child : children) {
            const QString fromChild = matchAuthRoot(QDir(child.absoluteFilePath()));
            if (!fromChild.isEmpty())
                return fromChild;
        }

        if (!dir.cdUp())
            break;
    }

    // Fallback por ubicaciones absolutas conocidas del proyecto, por si el
    // ejecutable (shadow-build de QtCreator) quedo en una carpeta desde la que
    // no se puede resolver la ruta del helper subiendo directorios.
    static const char* const kAbsoluteCandidates[] = {
        "C:/Carlens/Codigo-03/Codigo_Implementacion_IA",
        "C:/Carlens/Codigo-03",
    };
    for (const char* candidate : kAbsoluteCandidates) {
        const QString root = matchAuthRoot(QDir(QString::fromLatin1(candidate)));
        if (!root.isEmpty())
            return root;
    }

    // Ultimo recurso: ruta actual; executeHelper validara y reportara si no existe.
    return QCoreApplication::applicationDirPath();
}

QString RemoteAuthBackend::resolvePythonExe(const QString& projectRoot) const
{
    const QString venvPy = QDir(projectRoot).filePath(QStringLiteral(".venv/Scripts/python.exe"));
    if (QFileInfo::exists(venvPy))
        return venvPy;
    return QStringLiteral("python");
}

QString RemoteAuthBackend::resolveHelperPath(const QString& projectRoot) const
{
    const QString helper = QDir(projectRoot).filePath(QString::fromLatin1(kHelperRelPath));
    return QFileInfo::exists(helper) ? helper : QString();
}

QString RemoteAuthBackend::resolveConfigPath(const QString& projectRoot) const
{
    return QDir(projectRoot).filePath(QString::fromLatin1(kConfigRelPath));
}
