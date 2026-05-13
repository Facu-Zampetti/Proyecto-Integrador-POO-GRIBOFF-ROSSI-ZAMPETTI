#include "MainWindow.h"

#include <QByteArray>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPoint>
#include <QProcess>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      sourceEdit_(nullptr),
      showWindowCheck_(nullptr),
      startBtn_(nullptr),
      stopBtn_(nullptr),
            videoWidget_(nullptr),
      logView_(nullptr),
      statusLabel_(nullptr),
      process_(new QProcess(this)) {
    setupUi();

    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::startYolo);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::stopYolo);
    connect(process_, &QProcess::readyReadStandardOutput, this, &MainWindow::appendStdOut);
    connect(process_, &QProcess::readyReadStandardError, this, &MainWindow::appendStdErr);
    connect(process_, &QProcess::finished, this, &MainWindow::onProcessFinished);
    connect(videoWidget_, &VideoWidget::detectionClicked, this, &MainWindow::onDetectionClicked);
}

MainWindow::~MainWindow() {
    if (process_->state() != QProcess::NotRunning) {
        process_->kill();
        process_->waitForFinished(1500);
    }
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *sourceRow = new QHBoxLayout();
    auto *sourceLabel = new QLabel("Fuente de video:", this);
    sourceEdit_ = new QLineEdit("Video/autos.mp4", this);
    sourceRow->addWidget(sourceLabel);
    sourceRow->addWidget(sourceEdit_);

    auto *controlRow = new QHBoxLayout();
    showWindowCheck_ = new QCheckBox("Mostrar ventana de OpenCV", this);
    showWindowCheck_->setChecked(false);

    startBtn_ = new QPushButton("Iniciar YOLO", this);
    stopBtn_ = new QPushButton("Detener", this);
    stopBtn_->setEnabled(false);

    controlRow->addWidget(showWindowCheck_);
    controlRow->addStretch();
    controlRow->addWidget(startBtn_);
    controlRow->addWidget(stopBtn_);

    videoWidget_ = new VideoWidget(this);

    logView_ = new QTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setMaximumHeight(180);

    statusLabel_ = new QLabel("Listo para ejecutar", this);

    layout->addLayout(sourceRow);
    layout->addLayout(controlRow);
    layout->addWidget(videoWidget_, 1);
    layout->addWidget(logView_);
    layout->addWidget(statusLabel_);

    setCentralWidget(central);
    setWindowTitle("Carlens - Runner YOLO (Qt)");
}

QString MainWindow::findProjectRoot() const {
    QDir dir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 10; ++i) {
        const QString scriptPath = dir.filePath("backend/yolo_stream.py");
        if (QFileInfo::exists(scriptPath)) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return {};
}

QString MainWindow::resolvePythonExe(const QString &projectRoot) const {
    const QString venvPy = QDir(projectRoot).filePath(".venv/Scripts/python.exe");
    if (QFileInfo::exists(venvPy)) {
        return venvPy;
    }
    return QStringLiteral("python");
}

QString MainWindow::resolveScriptPath(const QString &projectRoot) const {
    return QDir(projectRoot).filePath("backend/yolo_stream.py");
}

void MainWindow::startYolo() {
    if (process_->state() != QProcess::NotRunning) {
        QMessageBox::information(this, "Ejecucion en curso", "YOLO ya esta ejecutandose.");
        return;
    }

    const QString projectRoot = findProjectRoot();
    if (projectRoot.isEmpty()) {
        QMessageBox::critical(
            this,
            "Ruta no encontrada",
            "No se pudo ubicar la raiz del proyecto (backend/yolo_stream.py)."
        );
        return;
    }

    projectRoot_ = projectRoot;
    stdOutBuffer_.clear();
    currentFrame_ = QImage();
    currentDetections_.clear();
    currentFrameId_ = 0;
    currentTimestamp_ = 0.0;
    videoWidget_->clearFrame();

    const QString pythonExe = resolvePythonExe(projectRoot);
    const QString scriptPath = resolveScriptPath(projectRoot);

    QString source = sourceEdit_->text().trimmed();
    if (source.isEmpty()) {
        QMessageBox::warning(this, "Fuente vacia", "Ingresa una ruta de video o RTSP.");
        return;
    }

    QFileInfo srcInfo(source);
    if (srcInfo.isRelative()) {
        source = QDir(projectRoot).filePath(source);
    }

    QStringList args;
    args << scriptPath << "--source" << source;
    if (!showWindowCheck_->isChecked()) {
        args << "--no-show" << "--emit-json";
    }

    process_->setProgram(pythonExe);
    process_->setArguments(args);
    process_->setWorkingDirectory(projectRoot);

    logView_->append(QString("[RUN] %1 %2").arg(pythonExe, args.join(' ')));
    process_->start();

    if (!process_->waitForStarted(4000)) {
        QMessageBox::critical(this, "Error al iniciar", "No se pudo iniciar el proceso Python.");
        return;
    }

    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    statusLabel_->setText("YOLO ejecutandose...");
}

void MainWindow::stopYolo() {
    if (process_->state() == QProcess::NotRunning) {
        return;
    }

    process_->terminate();
    if (!process_->waitForFinished(1500)) {
        process_->kill();
        process_->waitForFinished(1500);
    }
}

void MainWindow::appendStdOut() {
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

void MainWindow::appendStdErr() {
    const QString data = QString::fromUtf8(process_->readAllStandardError());
    if (!data.isEmpty()) {
        logView_->append(QString("[ERR] %1").arg(data.trimmed()));
    }
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);

    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    statusLabel_->setText(QString("Proceso finalizado (exit code %1)").arg(exitCode));
}

void MainWindow::onDetectionClicked(int detectionIndex, QPoint imagePoint) {
    Q_UNUSED(imagePoint);

    if (detectionIndex < 0 || detectionIndex >= currentDetections_.size()) {
        return;
    }

    if (saveDetectionCapture(currentDetections_.at(detectionIndex), imagePoint)) {
        statusLabel_->setText(QString("Capture guardada para %1 en frame %2")
                                  .arg(currentDetections_.at(detectionIndex).label)
                                  .arg(currentFrameId_));
    }
}

void MainWindow::handleStdOutLine(const QByteArray &line) {
    const QByteArray trimmedLine = line.trimmed();
    if (trimmedLine.isEmpty()) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(trimmedLine, &parseError);
    if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        const QJsonObject payload = jsonDoc.object();
        if (payload.contains("frame_jpeg_base64")) {
            processFramePayload(payload);
            return;
        }
    }

    logView_->append(QString::fromUtf8(trimmedLine));
}

void MainWindow::processFramePayload(const QJsonObject &payload) {
    const QByteArray encodedFrame = payload.value("frame_jpeg_base64").toString().toUtf8();
    const QByteArray jpegBytes = QByteArray::fromBase64(encodedFrame);

    QImage frame;
    if (!frame.loadFromData(jpegBytes, "JPG")) {
        logView_->append("[ERR] No se pudo decodificar el frame JPEG recibido.");
        return;
    }

    DetectionList detections;
    const QJsonArray jsonDetections = payload.value("detections").toArray();
    detections.reserve(jsonDetections.size());

    for (const QJsonValue &value : jsonDetections) {
        const QJsonObject detectionObject = value.toObject();
        const QJsonArray bboxArray = detectionObject.value("bbox").toArray();
        if (bboxArray.size() != 4) {
            continue;
        }

        const int x1 = bboxArray.at(0).toInt();
        const int y1 = bboxArray.at(1).toInt();
        const int x2 = bboxArray.at(2).toInt();
        const int y2 = bboxArray.at(3).toInt();

        DetectionBox detection;
        detection.classId = detectionObject.value("class_id").toInt(-1);
        detection.label = detectionObject.value("label").toString();
        detection.confidence = detectionObject.value("confidence").toDouble();
        detection.bbox = QRect(QPoint(x1, y1), QPoint(x2, y2)).normalized();
        if (detectionObject.contains("track_id") && !detectionObject.value("track_id").isNull()) {
            detection.trackId = static_cast<qint64>(detectionObject.value("track_id").toInteger(-1));
        }
        detections.push_back(detection);
    }

    currentFrame_ = frame;
    currentDetections_ = detections;
    currentFrameId_ = payload.value("frame_id").toInt();
    currentTimestamp_ = payload.value("timestamp").toDouble();

    const int sourceWidth = payload.value("width").toInt(frame.width());
    const int sourceHeight = payload.value("height").toInt(frame.height());
    videoWidget_->setFrame(currentFrame_, currentDetections_, sourceWidth, sourceHeight);
    statusLabel_->setText(QString("Frame %1 | Detecciones %2")
                              .arg(currentFrameId_)
                              .arg(currentDetections_.size()));
}

bool MainWindow::saveDetectionCapture(const DetectionBox &detection, const QPoint &imagePoint) {
    if (projectRoot_.isEmpty() || currentFrame_.isNull()) {
        logView_->append("[ERR] No hay un frame activo para guardar la captura.");
        return false;
    }

    const QRect imageBounds(QPoint(0, 0), currentFrame_.size());
    const QRect boundedBox = detection.bbox.intersected(imageBounds);
    if (!boundedBox.isValid() || boundedBox.isEmpty()) {
        logView_->append("[ERR] La bbox seleccionada esta fuera del frame.");
        return false;
    }

    QDir capturesDir(QDir(projectRoot_).filePath("captures"));
    if (!capturesDir.exists() && !QDir().mkpath(capturesDir.path())) {
        logView_->append("[ERR] No se pudo crear la carpeta captures/.");
        return false;
    }

    const QString safeLabel = detection.label.isEmpty() ? QStringLiteral("vehicle")
                                                        : QString(detection.label).replace(' ', '_');
    const QString timeToken = QDateTime::currentDateTimeUtc().toString("yyyyMMdd_hhmmss_zzz");
    const QString baseName = QString("capture_f%1_%2_%3")
                                 .arg(currentFrameId_, 6, 10, QChar('0'))
                                 .arg(safeLabel)
                                 .arg(timeToken);

    const QString imagePath = capturesDir.filePath(baseName + ".jpg");
    const QString metadataPath = capturesDir.filePath(baseName + ".json");

    const QImage crop = currentFrame_.copy(boundedBox);
    if (!crop.save(imagePath, "JPG", 95)) {
        logView_->append("[ERR] No se pudo guardar el crop seleccionado.");
        return false;
    }

    QJsonObject metadata;
    metadata.insert("frame_id", currentFrameId_);
    metadata.insert("timestamp", currentTimestamp_);
    metadata.insert("class_id", detection.classId);
    metadata.insert("label", detection.label);
    metadata.insert("confidence", detection.confidence);
    metadata.insert(
        "bbox",
        QJsonArray{boundedBox.left(), boundedBox.top(), boundedBox.right(), boundedBox.bottom()}
    );
    metadata.insert("image_width", currentFrame_.width());
    metadata.insert("image_height", currentFrame_.height());
    metadata.insert("saved_image_path", imagePath);
    metadata.insert("click_point", QJsonArray{imagePoint.x(), imagePoint.y()});
    if (detection.trackId >= 0) {
        metadata.insert("track_id", static_cast<qint64>(detection.trackId));
    }

    QFile metadataFile(metadataPath);
    if (!metadataFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logView_->append("[ERR] No se pudo guardar el metadata JSON.");
        return false;
    }

    metadataFile.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));
    metadataFile.close();

    logView_->append(QString("[CAPTURE] %1").arg(imagePath));
    return true;
}
