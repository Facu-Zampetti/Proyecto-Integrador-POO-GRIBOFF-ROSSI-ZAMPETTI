#include "widgets/VideoUploadWidget.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include "managers/SessionManager.h"
#include "managers/VideoManager.h"
#include "yolo/VideoWidget.h"
#include "yolo/YoloProcessController.h"
#include "yolo/CaptureManager.h"

VideoUploadWidget::VideoUploadWidget(QWidget* parent)
    : YoloPipelineWidget(parent)
{
    setAcceptDrops(true);
    setupUI();
    connectSignals();
}

VideoUploadWidget::~VideoUploadWidget() = default;

void VideoUploadWidget::setupUI()
{
    setObjectName("videoUploadWidget");
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    m_scrollContent = new QWidget(m_scrollArea);
    QVBoxLayout* contentLayout = new QVBoxLayout(m_scrollContent);
    contentLayout->setContentsMargins(32, 32, 32, 32);
    contentLayout->setSpacing(20);
    m_scrollArea->setWidget(m_scrollContent);
    rootLayout->addWidget(m_scrollArea);

    // ── Título de sección ─────────────────────────────────────────────
    QLabel* sectionTitle = new QLabel("Cargar Video para Análisis", m_scrollContent);
    QFont tf = sectionTitle->font();
    tf.setPointSize(18);
    tf.setWeight(QFont::DemiBold);
    sectionTitle->setFont(tf);
    sectionTitle->setStyleSheet("color: #e6edf3;");

    QLabel* subtitle = new QLabel(
        "Cargue un archivo MP4, AVI o MKV y observe la detección YOLO en vivo. "
        "Haga clic sobre un vehículo para capturarlo.",
        m_scrollContent);
    subtitle->setStyleSheet("color: #8b949e; font-size: 13px;");
    subtitle->setWordWrap(true);

    // ── Zona de drag & drop ───────────────────────────────────────────
    m_dropZone = new QWidget(m_scrollContent);
    m_dropZone->setObjectName("dropZone");
    m_dropZone->setMinimumHeight(160);
    m_dropZone->setAcceptDrops(true);
    m_dropZone->setStyleSheet(
        "#dropZone { "
        "  background-color: #161b22; "
        "  border: 2px dashed #30363d; "
        "  border-radius: 12px; "
        "}"
        "#dropZone:hover { border-color: #58a6ff; }");

    QVBoxLayout* dropLayout = new QVBoxLayout(m_dropZone);
    dropLayout->setAlignment(Qt::AlignCenter);
    dropLayout->setSpacing(12);

    QLabel* dropIcon = new QLabel(m_dropZone);
    dropIcon->setAlignment(Qt::AlignCenter);
    dropIcon->setFixedSize(64, 64);
    dropIcon->setPixmap(QIcon(":/icons/video-camera.svg").pixmap(QSize(64, 64)));

    m_dropLabel = new QLabel(
        "Arrastre un video aquí\no haga clic para seleccionar",
        m_dropZone);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
    m_dropLabel->setWordWrap(true);

    m_selectBtn = new QPushButton("Seleccionar Archivo", m_dropZone);
    m_selectBtn->setProperty("class", "accent");
    m_selectBtn->setMinimumWidth(180);
    m_selectBtn->setCursor(Qt::PointingHandCursor);

    dropLayout->addWidget(dropIcon);
    dropLayout->addWidget(m_dropLabel);
    dropLayout->addWidget(m_selectBtn, 0, Qt::AlignCenter);

    // ── Info del archivo seleccionado ─────────────────────────────────
    QWidget* fileInfoCard = new QWidget(m_scrollContent);
    fileInfoCard->setObjectName("fileInfoCard");
    fileInfoCard->setStyleSheet(
        "#fileInfoCard { background: #161b22; border: 1px solid #30363d; "
        "border-radius: 8px; padding: 12px; }");
    QHBoxLayout* fileInfoLayout = new QHBoxLayout(fileInfoCard);
    fileInfoLayout->setContentsMargins(12, 10, 12, 10);

    QLabel* fileIcon = new QLabel(fileInfoCard);
    fileIcon->setFixedSize(32, 32);
    fileIcon->setPixmap(QIcon(":/icons/film.svg").pixmap(QSize(32, 32)));

    QVBoxLayout* fileTextLayout = new QVBoxLayout();
    m_fileInfoLabel = new QLabel("Ningún archivo seleccionado", fileInfoCard);
    m_fileInfoLabel->setStyleSheet("color: #e6edf3; font-size: 13px; font-weight: 500;");
    m_fileSizeLabel = new QLabel("", fileInfoCard);
    m_fileSizeLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    fileTextLayout->addWidget(m_fileInfoLabel);
    fileTextLayout->addWidget(m_fileSizeLabel);

    m_clearBtn = new QPushButton("✕", fileInfoCard);
    m_clearBtn->setFixedSize(28, 28);
    m_clearBtn->setStyleSheet(
        "QPushButton { background: #21262d; border: none; border-radius: 14px; "
        "color: #8b949e; font-size: 14px; }"
        "QPushButton:hover { background: #da3633; color: white; }");
    m_clearBtn->setEnabled(false);

    fileInfoLayout->addWidget(fileIcon);
    fileInfoLayout->addLayout(fileTextLayout, 1);
    fileInfoLayout->addWidget(m_clearBtn);

    // ── Botones de proceso ─────────────────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout();
    m_analyzeBtn = new QPushButton("Analizar video", m_scrollContent);
    m_analyzeBtn->setProperty("class", "primary");
    m_analyzeBtn->setMinimumHeight(44);
    m_analyzeBtn->setMinimumWidth(160);
    m_analyzeBtn->setEnabled(false);
    m_analyzeBtn->setCursor(Qt::PointingHandCursor);
    QFont uf = m_analyzeBtn->font();
    uf.setWeight(QFont::DemiBold);
    m_analyzeBtn->setFont(uf);

    m_cancelBtn = new QPushButton("Cancelar", m_scrollContent);
    m_cancelBtn->setProperty("class", "danger");
    m_cancelBtn->setMinimumHeight(44);
    m_cancelBtn->setMinimumWidth(120);
    m_cancelBtn->setEnabled(false);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);

    btnRow->addWidget(m_analyzeBtn);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addStretch();

    // ── Estado del procesamiento ───────────────────────────────────────
    QWidget* statusCard = new QWidget(m_scrollContent);
    statusCard->setStyleSheet(
        "background: #161b22; border: 1px solid #30363d; border-radius: 8px;");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusCard);
    statusLayout->setContentsMargins(20, 12, 20, 12);
    statusLayout->setSpacing(10);

    m_statusDot = new QLabel(statusCard);
    m_statusDot->setFixedSize(12, 12);
    m_statusDot->setStyleSheet("background-color: #8b949e; border-radius: 6px;");

    m_statusLabel = new QLabel("Inactivo", statusCard);
    m_statusLabel->setStyleSheet("color: #8b949e; font-size: 13px; font-weight: 500;");

    m_timerLabel = new QLabel("", statusCard);
    m_timerLabel->setStyleSheet("color: #484f58; font-size: 12px;");

    m_sessionInfoLabel = new QLabel("", statusCard);
    m_sessionInfoLabel->setStyleSheet("color: #58a6ff; font-size: 12px;");

    statusLayout->addWidget(m_statusDot);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_timerLabel);
    statusLayout->addWidget(m_sessionInfoLabel);

    // ── Video con detecciones (VideoWidget real) ───────────────────────
    m_videoWidget = new VideoWidget(m_scrollContent);
    m_videoWidget->setMinimumHeight(320);

    // ── Ensamblar ─────────────────────────────────────────────────────
    contentLayout->addWidget(sectionTitle);
    contentLayout->addWidget(subtitle);
    contentLayout->addWidget(m_dropZone);
    contentLayout->addWidget(fileInfoCard);
    contentLayout->addLayout(btnRow);
    contentLayout->addWidget(statusCard);
    contentLayout->addWidget(m_videoWidget, 1);
    contentLayout->addStretch();
}

void VideoUploadWidget::connectSignals()
{
    connect(m_selectBtn,  &QPushButton::clicked, this, &VideoUploadWidget::onSelectFileClicked);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &VideoUploadWidget::onAnalyzeClicked);
    connect(m_cancelBtn,  &QPushButton::clicked, this, &VideoUploadWidget::onCancelClicked);
    connect(m_clearBtn,   &QPushButton::clicked, this, &VideoUploadWidget::reset);

    connectPipelineSignals();
}

// ─── Slots específicos de VideoUpload ────────────────────────────────────

void VideoUploadWidget::onSelectFileClicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Seleccionar Video",
        QDir::homePath(),
        "Videos (*.mp4 *.avi *.mkv);;Todos los archivos (*)"
    );
    if (!path.isEmpty())
        showFileInfo(path);
}

void VideoUploadWidget::onAnalyzeClicked()
{
    if (m_selectedFilePath.isEmpty())
        return;

    const QString validationError =
        VideoManager::instance()->validateFile(m_selectedFilePath);
    if (!validationError.isEmpty()) {
        emit errorMessage(validationError);
        return;
    }

    setStatus(Carlens::StreamStatus::Connecting);

    QString err;
    if (!m_yolo->start(m_selectedFilePath, /*showWindow=*/false, &err)) {
        setStatus(Carlens::StreamStatus::Error);
        emit errorMessage(err.isEmpty() ? "No se pudo iniciar YOLO." : err);
        return;
    }

    // Configurar la persistencia local con la raiz del proyecto resuelta por YOLO.
    m_captureManager->setProjectRoot(m_yolo->projectRoot());

    ProcessingRunInfo runInfo;
    runInfo.sourcePath    = m_selectedFilePath;
    runInfo.sourceType    = QFileInfo(m_selectedFilePath).suffix().toLower();
    runInfo.yoloModel     = "yolo11n.pt";
    runInfo.trackingModel = "manual_capture_no_tracker";
    runInfo.frameSkip     = 0;
    runInfo.confidenceThreshold = 0.35;
    runInfo.iouThreshold  = -1.0;
    runInfo.notes         = "Sesión iniciada desde la pestaña Cargar Video";
    runInfo.createdByUserId = SessionManager::instance()->currentUser().id();
    runInfo.startedAtUtc  = QDateTime::currentDateTimeUtc();

    // start-session abre el tunel SSH: en hilo de fondo para no congelar la UI.
    m_runActive = true;
    m_pendingRunOpLabel = "begin";
    CaptureManager* mgr = m_captureManager.get();
    QFuture<SnapshotPersistenceResult> future = QtConcurrent::run(
        [mgr, runInfo]() { return mgr->beginProcessingRun(runInfo); });
    m_runOpWatcher->setFuture(future);

    emit statusMessage("Procesando video: " + QFileInfo(m_selectedFilePath).fileName()
                       + " (estableciendo sesión…)");
}

void VideoUploadWidget::onCancelClicked()
{
    // Cancelación real: detiene el proceso YOLO de verdad.
    if (m_runActive && !m_runOpWatcher->isRunning()) {
        ProcessingRunCompletionInfo completion;
        completion.status     = "cancelled";
        completion.endedAtUtc = QDateTime::currentDateTimeUtc();
        m_runActive = false;
        m_pendingRunOpLabel = "finish";
        CaptureManager* mgr = m_captureManager.get();
        QFuture<SnapshotPersistenceResult> future = QtConcurrent::run(
            [mgr, completion]() { return mgr->finishProcessingRun(completion); });
        m_runOpWatcher->setFuture(future);
    } else {
        m_runActive = false;
    }

    m_yolo->stop();
    m_videoWidget->clearFrame();
    m_currentFrame = QImage();
    m_currentDetections.clear();
    setStatus(Carlens::StreamStatus::Disconnected);
    emit statusMessage("Procesamiento cancelado.");
}

// ─── Hooks del Template Method ────────────────────────────────────────────

QString VideoUploadWidget::statusToString(Carlens::StreamStatus status) const
{
    switch (status) {
    case Carlens::StreamStatus::Disconnected: return "Inactivo";
    case Carlens::StreamStatus::Connecting:   return "Iniciando…";
    case Carlens::StreamStatus::Connected:    return "Procesando — En vivo";
    case Carlens::StreamStatus::Error:        return "Error de procesamiento";
    }
    return "Desconocido";
}

void VideoUploadWidget::updateControls(Carlens::StreamStatus status)
{
    const bool connected  = (status == Carlens::StreamStatus::Connected);
    const bool connecting = (status == Carlens::StreamStatus::Connecting);
    const bool busy = connected || connecting;

    m_analyzeBtn->setEnabled(!busy && !m_selectedFilePath.isEmpty());
    m_cancelBtn->setEnabled(busy);
    m_selectBtn->setEnabled(!busy);
    m_clearBtn->setEnabled(!busy && !m_selectedFilePath.isEmpty());
}

// ─── Reset ────────────────────────────────────────────────────────────────

void VideoUploadWidget::reset()
{
    onCancelClicked();
    m_selectedFilePath.clear();
    m_fileInfoLabel->setText("Ningún archivo seleccionado");
    m_fileSizeLabel->clear();
    m_dropLabel->setText("Arrastre un video aquí\no haga clic para seleccionar");
    m_dropLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
    m_analyzeBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    m_clearBtn->setEnabled(false);
    setStatus(Carlens::StreamStatus::Disconnected);
    m_timer->stop();
    m_elapsedSeconds = 0;
    m_timerLabel->clear();
    m_sessionInfoLabel->clear();
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void VideoUploadWidget::showFileInfo(const QString& path)
{
    m_selectedFilePath = path;
    QFileInfo fi(path);
    m_fileInfoLabel->setText(fi.fileName());

    double sizeMB = fi.size() / (1024.0 * 1024.0);
    m_fileSizeLabel->setText(QString("%1 MB · %2")
                              .arg(sizeMB, 0, 'f', 2)
                              .arg(fi.suffix().toUpper()));
    m_dropLabel->setText("Archivo listo para analizar");
    m_dropLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
    m_analyzeBtn->setEnabled(true);
    m_clearBtn->setEnabled(true);
}

void VideoUploadWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const QString path = event->mimeData()->urls().first().toLocalFile();
        const QString ext  = QFileInfo(path).suffix().toLower();
        static const QStringList validExts = {"mp4", "avi", "mkv"};
        if (validExts.contains(ext)) {
            event->acceptProposedAction();
            m_dropZone->setStyleSheet(
                "#dropZone { background: #1f3a5f; border: 2px solid #58a6ff; "
                "border-radius: 12px; }");
            return;
        }
    }
    event->ignore();
}

void VideoUploadWidget::dropEvent(QDropEvent* event)
{
    // Restaurar el estilo por defecto de la zona de drop.
    m_dropZone->setStyleSheet(
        "#dropZone { background: #161b22; border: 2px dashed #30363d; "
        "border-radius: 12px; }"
        "#dropZone:hover { border-color: #58a6ff; }");

    if (event->mimeData()->hasUrls()) {
        const QString path = event->mimeData()->urls().first().toLocalFile();
        const QString ext  = QFileInfo(path).suffix().toLower();
        static const QStringList validExts = {"mp4", "avi", "mkv"};
        if (validExts.contains(ext)) {
            showFileInfo(path);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}
