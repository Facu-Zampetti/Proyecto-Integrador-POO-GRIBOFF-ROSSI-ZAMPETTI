#include "MainWindow.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFuture>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QTextEdit>
#include <QtConcurrent/QtConcurrentRun>
#include <QVBoxLayout>
#include <QWidget>

#include "FramePayload.h"
#include "YoloProcessController.h"

namespace {
constexpr double kMinimumCaptureConfidence = CaptureManager::minimumConfidenceForCapture();

QString resolveSourceForPersistence(const QString &source, const QString &projectRoot) {
    QFileInfo sourceInfo(source);
    if (sourceInfo.isRelative() && !projectRoot.isEmpty()) {
        return QDir(projectRoot).filePath(source);
    }
    return source;
}

QString inferSourceType(const QString &source) {
    const QString trimmed = source.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("file");
    }
    if (trimmed.toInt() >= 0 && QString::number(trimmed.toInt()) == trimmed) {
        return QStringLiteral("camera");
    }
    if (trimmed.startsWith(QStringLiteral("rtsp://"), Qt::CaseInsensitive)) {
        return QStringLiteral("rtsp");
    }
    const QString suffix = QFileInfo(trimmed).suffix().toLower();
    return suffix.isEmpty() ? QStringLiteral("file") : suffix;
}

bool confirmSnapshotAnalysis(QWidget *parent, const CapturedVehicleSnapshot &snapshot) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Confirmar captura"));
    dialog.setModal(true);
    dialog.resize(420, 520);

    auto *layout = new QVBoxLayout(&dialog);

    auto *titleLabel = new QLabel(
        QStringLiteral("Revisa la captura antes de enviarla a analisis."),
        &dialog
    );
    titleLabel->setWordWrap(true);

    auto *detailsLabel = new QLabel(
        QStringLiteral("Vehiculo: %1\nConfianza: %2\nFrame: %3")
            .arg(snapshot.label.isEmpty() ? QStringLiteral("vehiculo") : snapshot.label)
            .arg(snapshot.confidence, 0, 'f', 2)
            .arg(snapshot.frameId),
        &dialog
    );
    detailsLabel->setWordWrap(true);

    auto *previewLabel = new QLabel(&dialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumSize(320, 240);
    previewLabel->setStyleSheet(QStringLiteral("background-color: #111; border: 1px solid #444;"));

    if (!snapshot.croppedImage.isNull()) {
        const QPixmap previewPixmap = QPixmap::fromImage(snapshot.croppedImage).scaled(
            QSize(380, 320),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        previewLabel->setPixmap(previewPixmap);
    } else {
        previewLabel->setText(QStringLiteral("No se pudo generar la vista previa."));
    }

    auto *questionLabel = new QLabel(
        QStringLiteral("Si eliges Analizar, la captura se guarda y queda lista para el analisis por IA mas adelante. Si eliges Descartar, se elimina del flujo y el video continua sin guardar nada."),
        &dialog
    );
    questionLabel->setWordWrap(true);

    auto *buttonBox = new QDialogButtonBox(&dialog);
    QPushButton *analyzeButton = buttonBox->addButton(
        QStringLiteral("Analizar"),
        QDialogButtonBox::AcceptRole
    );
    QPushButton *discardButton = buttonBox->addButton(
        QStringLiteral("Descartar"),
        QDialogButtonBox::RejectRole
    );

    QObject::connect(analyzeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(discardButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    layout->addWidget(titleLabel);
    layout->addWidget(detailsLabel);
    layout->addWidget(previewLabel, 1);
    layout->addWidget(questionLabel);
    layout->addWidget(buttonBox);

    return dialog.exec() == QDialog::Accepted;
}

void appendPersistenceMessage(QTextEdit *logView, QLabel *statusLabel, const SnapshotPersistenceResult &result) {
    if (!result.warningMessage.isEmpty()) {
        logView->append(QString("[WARN] %1").arg(result.warningMessage));
        statusLabel->setText(result.warningMessage);
    }
    if (!result.errorMessage.isEmpty()) {
        logView->append(result.errorMessage);
        statusLabel->setText(result.errorMessage);
    }
    if (!result.infoMessage.isEmpty()) {
        logView->append(QString("[INFO] %1").arg(result.infoMessage));
        if (result.errorMessage.isEmpty() && result.warningMessage.isEmpty()) {
            statusLabel->setText(result.infoMessage);
        }
    }
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      sourceEdit_(nullptr),
      showWindowCheck_(nullptr),
      startBtn_(nullptr),
      stopBtn_(nullptr),
            videoWidget_(nullptr),
      logView_(nullptr),
      statusLabel_(nullptr),
      processController_(new YoloProcessController(this)),
      captureManager_(std::make_unique<CaptureManager>()),
            capturePersistWatcher_(new QFutureWatcher<CaptureResult>(this)),
            vehicleAnalysisWatcher_(new QFutureWatcher<VehicleAnalysisResult>(this)) {
    setupUi();

    connect(startBtn_, &QPushButton::clicked, this, &MainWindow::startYolo);
    connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::stopYolo);
    connect(processController_, &YoloProcessController::logMessage, this, &MainWindow::appendLogMessage);
    connect(processController_, &YoloProcessController::frameReady, this, &MainWindow::onFrameReady);
    connect(processController_, &YoloProcessController::runningChanged, this, &MainWindow::onRunningChanged);
    connect(processController_, &YoloProcessController::processFinished, this, &MainWindow::onProcessFinished);
    connect(
        capturePersistWatcher_,
        &QFutureWatcher<CaptureResult>::finished,
        this,
        &MainWindow::onCapturePersistenceFinished
    );
    connect(
        vehicleAnalysisWatcher_,
        &QFutureWatcher<VehicleAnalysisResult>::finished,
        this,
        &MainWindow::onVehicleAnalysisFinished
    );
    connect(videoWidget_, &VideoWidget::detectionClicked, this, &MainWindow::onDetectionClicked);
}

MainWindow::~MainWindow() {
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

void MainWindow::startYolo() {
    if (processController_->isRunning()) {
        QMessageBox::information(this, "Ejecucion en curso", "YOLO ya esta ejecutandose.");
        return;
    }

    currentFrame_ = QImage();
    currentDetections_.clear();
    currentFrameId_ = 0;
    currentTimestamp_ = 0.0;
    videoWidget_->clearFrame();

    const QString source = sourceEdit_->text().trimmed();
    if (source.isEmpty()) {
        QMessageBox::warning(this, "Fuente vacia", "Ingresa una ruta de video o RTSP.");
        return;
    }

    QString errorMessage;
    if (!processController_->start(source, showWindowCheck_->isChecked(), &errorMessage)) {
        const QString title = errorMessage.contains("raiz del proyecto") ? "Ruta no encontrada"
                                                                          : "Error al iniciar";
        QMessageBox::critical(this, title, errorMessage);
        return;
    }

    captureManager_->setProjectRoot(processController_->projectRoot());

    ProcessingRunInfo runInfo;
    runInfo.sourcePath = resolveSourceForPersistence(source, processController_->projectRoot());
    runInfo.sourceType = inferSourceType(source);
    runInfo.yoloModel = QStringLiteral("yolo11n.pt");
    runInfo.trackingModel = QStringLiteral("manual_capture_no_tracker");
    runInfo.frameSkip = 0;
    runInfo.confidenceThreshold = 0.35;
    runInfo.iouThreshold = -1.0;
    runInfo.notes = QStringLiteral("Sesion iniciada desde la app Qt desktop");
    runInfo.startedAtUtc = QDateTime::currentDateTimeUtc();

    const SnapshotPersistenceResult persistenceResult = captureManager_->beginProcessingRun(runInfo);
    appendPersistenceMessage(logView_, statusLabel_, persistenceResult);
}

void MainWindow::stopYolo() {
    ProcessingRunCompletionInfo completionInfo;
    completionInfo.status = QStringLiteral("stopped");
    completionInfo.endedAtUtc = QDateTime::currentDateTimeUtc();

    if (captureConfirmationActive_ || capturePersistenceInProgress_) {
        deferredProcessingCompletionInfo_ = completionInfo;
        hasDeferredProcessingCompletion_ = true;
    } else {
        appendPersistenceMessage(logView_, statusLabel_, captureManager_->finishProcessingRun(completionInfo));
    }

    processController_->stop();
}

void MainWindow::appendLogMessage(const QString &message) {
    if (!message.isEmpty()) {
        logView_->append(message);
    }
}

void MainWindow::onFrameReady(const FramePayload &payload) {
    currentFrame_ = payload.frame;
    currentDetections_ = payload.detections;
    currentFrameId_ = payload.frameId;
    currentTimestamp_ = payload.timestamp;

    videoWidget_->setFrame(
        currentFrame_,
        currentDetections_,
        payload.sourceWidth,
        payload.sourceHeight
    );
    statusLabel_->setText(QString("Frame %1 | Detecciones %2")
                              .arg(currentFrameId_)
                              .arg(currentDetections_.size()));
}

void MainWindow::onRunningChanged(bool running, const QString &statusMessage) {
    startBtn_->setEnabled(!running);
    stopBtn_->setEnabled(running);
    statusLabel_->setText(statusMessage);
}

void MainWindow::onProcessFinished(int exitCode) {
    ProcessingRunCompletionInfo completionInfo;
    completionInfo.status = exitCode == 0 ? QStringLiteral("completed")
                                          : QStringLiteral("failed");
    completionInfo.endedAtUtc = QDateTime::currentDateTimeUtc();

    if (captureConfirmationActive_ || capturePersistenceInProgress_) {
        deferredProcessingCompletionInfo_ = completionInfo;
        hasDeferredProcessingCompletion_ = true;
        return;
    }

    appendPersistenceMessage(logView_, statusLabel_, captureManager_->finishProcessingRun(completionInfo));
}

void MainWindow::onDetectionClicked(int detectionIndex, QPoint imagePoint) {
    if (detectionIndex < 0 || detectionIndex >= currentDetections_.size()) {
        return;
    }

    if (capturePersistenceInProgress_ || vehicleAnalysisWatcher_->isRunning()) {
        const QString busyMessage = QStringLiteral(
            "Ya hay una captura procesandose. Espera a que termine el guardado o el analisis."
        );
        statusLabel_->setText(busyMessage);
        logView_->append(QString("[INFO] %1").arg(busyMessage));
        return;
    }

    const DetectionBox &selectedDetection = currentDetections_.at(detectionIndex);
    if (selectedDetection.confidence < kMinimumCaptureConfidence) {
        const QString warningMessage =
            QStringLiteral("La deteccion no tiene suficiente confianza para capturar.");
        statusLabel_->setText(warningMessage);
        logView_->append(
            QString("[WARN] %1 (confianza %2)")
                .arg(warningMessage)
                .arg(selectedDetection.confidence, 0, 'f', 2)
        );
        QMessageBox::information(this, "Captura no permitida", warningMessage);
        return;
    }

    CaptureResult preparedResult = captureManager_->prepareDetection(
        currentFrame_,
        currentFrameId_,
        currentTimestamp_,
        selectedDetection,
        imagePoint
    );
    if (!preparedResult.success) {
        if (!preparedResult.errorMessage.isEmpty()) {
            logView_->append(preparedResult.errorMessage);
            statusLabel_->setText(preparedResult.errorMessage);
        }
        return;
    }

    captureConfirmationActive_ = true;
    const bool shouldPersist = confirmSnapshotAnalysis(this, preparedResult.snapshot);
    captureConfirmationActive_ = false;

    if (!shouldPersist) {
        const QString discardMessage = QStringLiteral(
            "Captura descartada por el usuario. El video continua sin guardar la imagen."
        );
        logView_->append(QString("[DISCARD] %1").arg(discardMessage));
        statusLabel_->setText(discardMessage);
        finalizeDeferredProcessingRun();
        return;
    }

    capturePersistenceInProgress_ = true;
    statusLabel_->setText(QStringLiteral("Guardando captura en segundo plano..."));

    QFuture<CaptureResult> future = QtConcurrent::run(
        [this, snapshot = std::move(preparedResult.snapshot)]() mutable {
            return captureManager_->persistSnapshot(std::move(snapshot));
        }
    );
    capturePersistWatcher_->setFuture(future);
}

void MainWindow::finalizeDeferredProcessingRun() {
    if (!hasDeferredProcessingCompletion_) {
        return;
    }

    hasDeferredProcessingCompletion_ = false;
    appendPersistenceMessage(
        logView_,
        statusLabel_,
        captureManager_->finishProcessingRun(deferredProcessingCompletionInfo_)
    );
}

void MainWindow::onCapturePersistenceFinished() {
    capturePersistenceInProgress_ = false;

    const CaptureResult result = capturePersistWatcher_->result();
    if (result.success) {
        logView_->append(QString("[CAPTURE] %1").arg(result.snapshot.localImagePath));
        if (!result.snapshot.remoteFilePath.isEmpty()) {
            logView_->append(QString("[REMOTE] %1").arg(result.snapshot.remoteFilePath));
        }
        if (!result.warningMessage.isEmpty()) {
            logView_->append(QString("[WARN] %1").arg(result.warningMessage));
        }
        if (!result.infoMessage.isEmpty()) {
            logView_->append(QString("[INFO] %1").arg(result.infoMessage));
        }
        statusLabel_->setText(QStringLiteral("Captura guardada. Iniciando analisis Gemini..."));

        const QString projectRoot = processController_->projectRoot();
        QFuture<VehicleAnalysisResult> future = QtConcurrent::run(
            [this, snapshot = result.snapshot, projectRoot]() {
                return geminiAnalysisService_.analyzeSnapshot(snapshot, projectRoot);
            }
        );
        vehicleAnalysisWatcher_->setFuture(future);
        finalizeDeferredProcessingRun();
    } else if (!result.errorMessage.isEmpty()) {
        logView_->append(result.errorMessage);
        statusLabel_->setText(result.errorMessage);
        finalizeDeferredProcessingRun();
    }
}

void MainWindow::onVehicleAnalysisFinished() {
    const VehicleAnalysisResult result = vehicleAnalysisWatcher_->result();

    if (result.success) {
        if (!result.infoMessage.isEmpty()) {
            logView_->append(result.infoMessage);
        }

        const QString description = QStringLiteral(
            "[AI] %1 %2 | color=%3 | tipo=%4 | anio=%5 | confianza=%6"
        )
                                       .arg(result.make.isEmpty() ? QStringLiteral("?") : result.make)
                                       .arg(result.model.isEmpty() ? QStringLiteral("?") : result.model)
                                       .arg(result.color.isEmpty() ? QStringLiteral("?") : result.color)
                                       .arg(result.bodyType.isEmpty() ? QStringLiteral("?") : result.bodyType)
                                       .arg(result.yearRange.isEmpty() ? QStringLiteral("?") : result.yearRange)
                                       .arg(
                                           result.confidence >= 0.0
                                               ? QString::number(result.confidence, 'f', 2)
                                               : QStringLiteral("?")
                                       );
        logView_->append(description);
        if (!result.summary.isEmpty()) {
            logView_->append(QString("[AI] %1").arg(result.summary));
        }
        if (!result.outputPath.isEmpty()) {
            logView_->append(QString("[AI-FILE] %1").arg(result.outputPath));
        }
        if (!result.remoteOutputPath.isEmpty()) {
            logView_->append(QString("[AI-REMOTE] %1").arg(result.remoteOutputPath));
        }
        if (!result.warningMessage.isEmpty()) {
            logView_->append(result.warningMessage);
        }
        statusLabel_->setText(QStringLiteral("Analisis Gemini completado."));
    } else if (!result.warningMessage.isEmpty()) {
        logView_->append(result.warningMessage);
        statusLabel_->setText(result.warningMessage);
    } else if (!result.errorMessage.isEmpty()) {
        logView_->append(result.errorMessage);
        statusLabel_->setText(result.errorMessage);
    }
}
