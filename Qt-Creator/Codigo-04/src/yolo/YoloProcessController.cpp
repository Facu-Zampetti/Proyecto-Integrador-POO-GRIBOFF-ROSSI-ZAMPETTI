#include "YoloProcessController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QProcess>

#include "FramePayloadParser.h"

YoloProcessController::YoloProcessController(QObject *parent)
    : QObject(parent),
      process_(new QProcess(this)) {
    connect(process_, &QProcess::readyReadStandardOutput, this, &YoloProcessController::appendStdOut);
    connect(process_, &QProcess::readyReadStandardError, this, &YoloProcessController::appendStdErr);
    connect(process_, &QProcess::finished, this, &YoloProcessController::onProcessFinished);
}

YoloProcessController::~YoloProcessController() {
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1500);
    }
}

bool YoloProcessController::isRunning() const {
    return process_->state() != QProcess::NotRunning;
}

bool YoloProcessController::start(const QString &source, bool showWindow, QString *errorMessage) {
    if (isRunning()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("YOLO ya esta ejecutandose.");
        }
        return false;
    }

    const QString resolvedProjectRoot = findProjectRoot();
    if (resolvedProjectRoot.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral(
                "No se pudo ubicar la raiz del proyecto (backend/yolo_stream.py)."
            );
        }
        return false;
    }

    QString normalizedSource = source.trimmed();
    if (normalizedSource.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Ingresa una ruta de video o RTSP.");
        }
        return false;
    }

    QFileInfo sourceInfo(normalizedSource);
    if (sourceInfo.isRelative()) {
        normalizedSource = QDir(resolvedProjectRoot).filePath(normalizedSource);
    }

    projectRoot_ = resolvedProjectRoot;
    stdOutBuffer_.clear();

    const QString pythonExe = resolvePythonExe(projectRoot_);
    const QString scriptPath = resolveScriptPath(projectRoot_);

    QStringList args;
    args << scriptPath << "--source" << normalizedSource;
    if (!showWindow) {
        args << "--no-show" << "--emit-json";
    }

    process_->setProgram(pythonExe);
    process_->setArguments(args);
    process_->setWorkingDirectory(projectRoot_);

    emit logMessage(QString("[RUN] %1 %2").arg(pythonExe, args.join(' ')));
    process_->start();

    if (!process_->waitForStarted(4000)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No se pudo iniciar el proceso Python.");
        }
        projectRoot_.clear();
        return false;
    }

    emit runningChanged(true, QStringLiteral("YOLO ejecutandose..."));
    return true;
}

void YoloProcessController::stop() {
    if (!isRunning()) {
        return;
    }

    process_->terminate();
    if (!process_->waitForFinished(1500)) {
        process_->kill();
        process_->waitForFinished(1500);
    }
}

QString YoloProcessController::projectRoot() const {
    return projectRoot_;
}

void YoloProcessController::appendStdOut() {
    stdOutBuffer_.append(process_->readAllStandardOutput());

    qsizetype newlineIndex = -1;
    while ((newlineIndex = stdOutBuffer_.indexOf('\n')) >= 0) {
        QByteArray line = stdOutBuffer_.left(newlineIndex);
        stdOutBuffer_.remove(0, newlineIndex + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        handleStdOutLine(line);
    }
}

void YoloProcessController::appendStdErr() {
    const QString data = QString::fromUtf8(process_->readAllStandardError()).trimmed();
    if (!data.isEmpty()) {
        emit logMessage(QString("[ERR] %1").arg(data));
    }
}

void YoloProcessController::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);

    emit runningChanged(false, QString("Proceso finalizado (exit code %1)").arg(exitCode));
    emit processFinished(exitCode);
}

// Devuelve la raiz que contiene backend/yolo_stream.py a partir de un dir base:
// el propio base, o base/Codigo_Implementacion_IA. Cadena vacia si nada.
static QString matchYoloRoot(const QDir &base) {
    if (QFileInfo::exists(base.filePath("backend/yolo_stream.py"))) {
        return base.absolutePath();
    }
    const QString nested = base.filePath("Codigo_Implementacion_IA/backend/yolo_stream.py");
    if (QFileInfo::exists(nested)) {
        return QDir(base.filePath("Codigo_Implementacion_IA")).absolutePath();
    }
    return {};
}

QString YoloProcessController::findProjectRoot() const {
    QDir dir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 10; ++i) {
        // 1) El propio ancestro (o su subdir Codigo_Implementacion_IA).
        const QString direct = matchYoloRoot(dir);
        if (!direct.isEmpty()) {
            return direct;
        }

        // 2) Un nivel hacia abajo: cubre el shadow-build de QtCreator, que queda
        //    como carpeta hermana del proyecto (p.ej. C:/Carlens/build-... con
        //    el proyecto en C:/Carlens/Codigo-03/Codigo_Implementacion_IA).
        const QFileInfoList children = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &child : children) {
            const QString fromChild = matchYoloRoot(QDir(child.absoluteFilePath()));
            if (!fromChild.isEmpty()) {
                return fromChild;
            }
        }

        if (!dir.cdUp()) {
            break;
        }
    }

    // Fallback por ubicaciones absolutas conocidas del proyecto, por si el
    // ejecutable quedo en una carpeta desde la que no se puede resolver subiendo.
    static const char *const kAbsoluteCandidates[] = {
        "C:/Carlens/Codigo-03/Codigo_Implementacion_IA",
        "C:/Carlens/Codigo-03",
    };
    for (const char *candidate : kAbsoluteCandidates) {
        const QString root = matchYoloRoot(QDir(QString::fromLatin1(candidate)));
        if (!root.isEmpty()) {
            return root;
        }
    }

    return {};
}

QString YoloProcessController::resolvePythonExe(const QString &projectRoot) const {
    const QString venvPy = QDir(projectRoot).filePath(".venv/Scripts/python.exe");
    if (QFileInfo::exists(venvPy)) {
        return venvPy;
    }
    return QStringLiteral("python");
}

QString YoloProcessController::resolveScriptPath(const QString &projectRoot) const {
    return QDir(projectRoot).filePath("backend/yolo_stream.py");
}

void YoloProcessController::handleStdOutLine(const QByteArray &line) {
    const QByteArray trimmedLine = line.trimmed();
    if (trimmedLine.isEmpty()) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(trimmedLine, &parseError);
    if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        const QJsonObject payload = jsonDoc.object();
        if (payload.contains("frame_jpeg_base64")) {
            FramePayload framePayload;
            QString errorMessage;
            if (FramePayloadParser::parse(payload, &framePayload, &errorMessage)) {
                emit frameReady(framePayload);
            } else {
                emit logMessage(QString("[ERR] %1").arg(errorMessage));
            }
            return;
        }
    }

    emit logMessage(QString::fromUtf8(trimmedLine));
}