#include "widgets/VideoUploadWidget.h"
#include "managers/VideoManager.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QGraphicsDropShadowEffect>

namespace {
QString previewStatusMessage(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::NoMedia:
        return "";
    case QMediaPlayer::LoadingMedia:
    case QMediaPlayer::BufferingMedia:
        return "Preparando vista previa del video...";
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        return "Vista previa lista.";
    case QMediaPlayer::EndOfMedia:
        return "Vista previa finalizada. Puede reproducirla nuevamente.";
    case QMediaPlayer::InvalidMedia:
        return "No se pudo cargar el video seleccionado.";
    default:
        return "";
    }
}
}

VideoUploadWidget::VideoUploadWidget(QWidget* parent)
    : BaseWidget(parent)
{
    setAcceptDrops(true);
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.65f);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    setupUI();
    connectSignals();
}

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
        "Cargue un archivo MP4, AVI o MKV para detectar vehículos con YOLOv11.",
        m_scrollContent);
    subtitle->setStyleSheet("color: #8b949e; font-size: 13px;");
    subtitle->setWordWrap(true);

    // ── Zona de drag & drop ───────────────────────────────────────────
    m_dropZone = new QWidget(m_scrollContent);
    m_dropZone->setObjectName("dropZone");
    m_dropZone->setMinimumHeight(200);
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

    QLabel* dropIcon = new QLabel("📹", m_dropZone);
    dropIcon->setAlignment(Qt::AlignCenter);
    QFont iconFont = dropIcon->font();
    iconFont.setPointSize(40);
    dropIcon->setFont(iconFont);

    m_dropLabel = new QLabel(
        "Arrastre un video aquí\no haga clic para seleccionar",
        m_dropZone);
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
    m_dropLabel->setWordWrap(true);

    m_selectBtn = new QPushButton("Seleccionar Archivo", m_dropZone);
    m_selectBtn->setProperty("class", "accent");
    m_selectBtn->setFixedWidth(180);
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

    QLabel* fileIcon = new QLabel("🎬", fileInfoCard);
    fileIcon->setStyleSheet("font-size: 24px;");

    QVBoxLayout* fileTextLayout = new QVBoxLayout();
    m_fileInfoLabel = new QLabel("Ningún archivo seleccionado", fileInfoCard);
    m_fileInfoLabel->setStyleSheet("color: #e6edf3; font-size: 13px; font-weight: 500;");
    m_fileSizeLabel = new QLabel("", fileInfoCard);
    m_fileSizeLabel->setStyleSheet("color: #8b949e; font-size: 11px;");
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

    // ── Progreso ──────────────────────────────────────────────────────
    QWidget* progressSection = new QWidget(m_scrollContent);
    QHBoxLayout* progressLayout = new QHBoxLayout(progressSection);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(12);

    m_progressBar = new QProgressBar(m_scrollContent);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setVisible(false);

    m_percentLabel = new QLabel("0%", m_scrollContent);
    m_percentLabel->setStyleSheet("color: #58a6ff; font-size: 12px;");
    m_percentLabel->setVisible(false);

    progressLayout->addWidget(m_progressBar, 1);
    progressLayout->addWidget(m_percentLabel);

    // ── Botón de upload ───────────────────────────────────────────────
    m_uploadBtn = new QPushButton("  Enviar al Backend", m_scrollContent);
    m_uploadBtn->setProperty("class", "primary");
    m_uploadBtn->setMinimumHeight(44);
    m_uploadBtn->setEnabled(false);
    m_uploadBtn->setCursor(Qt::PointingHandCursor);
    QFont uf = m_uploadBtn->font();
    uf.setWeight(QFont::DemiBold);
    m_uploadBtn->setFont(uf);

    // ── Card de resultado ─────────────────────────────────────────────
    m_resultCard = new QWidget(m_scrollContent);
    m_resultCard->setStyleSheet(
        "background: #0d2818; border: 1px solid #2ea043; border-radius: 8px; padding: 12px;");
    m_resultCard->setVisible(false);
    QVBoxLayout* resultLayout = new QVBoxLayout(m_resultCard);
    m_resultLabel = new QLabel(m_resultCard);
    m_resultLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
    m_resultLabel->setWordWrap(true);
    resultLayout->addWidget(m_resultLabel);

    m_playBtn = new QPushButton("Reproducir video cargado", m_resultCard);
    m_playBtn->setProperty("class", "accent");
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setVisible(false);
    resultLayout->addWidget(m_playBtn, 0, Qt::AlignLeft);

    m_previewCard = new QWidget(m_scrollContent);
    m_previewCard->setStyleSheet(
        "background: #161b22; border: 1px solid #30363d; border-radius: 8px; padding: 12px;");
    m_previewCard->setVisible(false);
    QVBoxLayout* previewLayout = new QVBoxLayout(m_previewCard);

    QLabel* previewTitle = new QLabel("Vista previa automática", m_previewCard);
    previewTitle->setStyleSheet("color: #e6edf3; font-size: 13px; font-weight: 600;");
    previewLayout->addWidget(previewTitle);

    m_previewVideo = new QVideoWidget(m_previewCard);
    m_previewVideo->setMinimumHeight(220);
    m_previewVideo->setStyleSheet("background: #010409; border-radius: 8px;");
    m_previewVideo->setAspectRatioMode(Qt::KeepAspectRatio);
    previewLayout->addWidget(m_previewVideo);

    m_previewInfoLabel = new QLabel("", m_previewCard);
    m_previewInfoLabel->setStyleSheet("color: #8b949e; font-size: 12px;");
    m_previewInfoLabel->setWordWrap(true);
    previewLayout->addWidget(m_previewInfoLabel);

    m_mediaPlayer->setVideoOutput(m_previewVideo);

    // ── Ensamblar ─────────────────────────────────────────────────────
    contentLayout->addWidget(sectionTitle);
    contentLayout->addWidget(subtitle);
    contentLayout->addWidget(m_dropZone);
    contentLayout->addWidget(fileInfoCard);
    contentLayout->addWidget(progressSection);
    contentLayout->addWidget(m_uploadBtn);
    contentLayout->addWidget(m_resultCard);
    contentLayout->addWidget(m_previewCard);
    contentLayout->addStretch();
}

void VideoUploadWidget::connectSignals()
{
    connect(m_selectBtn, &QPushButton::clicked,
            this,        &VideoUploadWidget::onSelectFileClicked);
    connect(m_uploadBtn, &QPushButton::clicked,
            this,        &VideoUploadWidget::onUploadClicked);
    connect(m_clearBtn, &QPushButton::clicked,
            this,       &VideoUploadWidget::reset);
    connect(m_playBtn, &QPushButton::clicked,
            this,      &VideoUploadWidget::onPlayUploadedVideoClicked);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
                const QString message = previewStatusMessage(status);
                if (!message.isEmpty())
                    m_previewInfoLabel->setText(message);

                if (status == QMediaPlayer::EndOfMedia) {
                    m_mediaPlayer->stop();
                    m_mediaPlayer->setPosition(0);
                    m_playBtn->setText("Reproducir nuevamente");
                    m_playBtn->setEnabled(true);
                }
            });
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error, const QString& errorString) {
                if (!errorString.isEmpty()) {
                    m_previewInfoLabel->setText(errorString);
                    emit errorMessage("No se pudo reproducir el video automáticamente: " + errorString);
                }
            });

    auto* vm = VideoManager::instance();
    connect(vm, &VideoManager::uploadStarted,        this, &VideoUploadWidget::onUploadStarted);
    connect(vm, &VideoManager::uploadProgressChanged, this, &VideoUploadWidget::onProgressChanged);
    connect(vm, &VideoManager::uploadCompleted,       this, &VideoUploadWidget::onUploadCompleted);
    connect(vm, &VideoManager::uploadFailed,          this, &VideoUploadWidget::onUploadFailed);
}

// ─── Slots ────────────────────────────────────────────────────────────────

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

void VideoUploadWidget::onUploadClicked()
{
    if (m_selectedFilePath.isEmpty()) return;
    m_resultCard->setVisible(false);
    m_playBtn->setVisible(false);
    m_playBtn->setText("Reproducir video cargado");
    m_previewStartedForCurrentUpload = false;
    m_previewCard->setVisible(false);
    m_previewInfoLabel->setText("La vista previa se reproducirá automáticamente al finalizar la transferencia.");
    VideoManager::instance()->uploadVideo(m_selectedFilePath);
}

void VideoUploadWidget::onPlayUploadedVideoClicked()
{
    playUploadedVideo();
    m_playBtn->setText("Reproducir nuevamente");
}

void VideoUploadWidget::onUploadStarted(const QString& filename)
{
    setUploadState(Carlens::UploadStatus::Uploading);
    stopPreviewPlayback();
    m_previewStartedForCurrentUpload = false;
    m_playBtn->setEnabled(false);
    m_previewInfoLabel->setText("Subiendo video y preparando vista previa...");
    emit statusMessage("Subiendo: " + filename + "…");
}

void VideoUploadWidget::onProgressChanged(int percent)
{
    m_progressBar->setValue(percent);
    m_percentLabel->setText(QString("%1%").arg(percent));

    if (percent >= 100 && !m_previewStartedForCurrentUpload)
        playUploadedVideo();
}

void VideoUploadWidget::onUploadCompleted(const SessionModel& session)
{
    setUploadState(Carlens::UploadStatus::Completed);
    QString msg = QString(
        "✅ Video procesado correctamente.\n"
        "Sesión ID: %1 · Vehículos detectados: %2")
        .arg(session.id())
        .arg(session.totalVehicles());
    m_resultLabel->setText(msg);
    m_resultCard->setVisible(true);
    m_playBtn->setVisible(QFileInfo::exists(m_selectedFilePath));
    m_playBtn->setEnabled(true);
    if (!m_previewStartedForCurrentUpload)
        playUploadedVideo();
    m_previewInfoLabel->setText("Video reproducido automáticamente. Puede seguir viéndolo mientras revisa el resultado.");
    emit successMessage(QString("Procesamiento completado: %1 vehículos detectados.")
                        .arg(session.totalVehicles()));
}

void VideoUploadWidget::onUploadFailed(const QString& reason)
{
    setUploadState(Carlens::UploadStatus::Failed);
    emit errorMessage(reason);
}

// ─── Helpers ──────────────────────────────────────────────────────────────

void VideoUploadWidget::showFileInfo(const QString& path)
{
    stopPreviewPlayback();
    m_previewStartedForCurrentUpload = false;
    m_selectedFilePath = path;
    QFileInfo fi(path);
    m_fileInfoLabel->setText(fi.fileName());

    double sizeMB = fi.size() / (1024.0 * 1024.0);
    m_fileSizeLabel->setText(QString("%1 MB · %2")
                              .arg(sizeMB, 0, 'f', 2)
                              .arg(fi.suffix().toUpper()));
    m_dropLabel->setText("Archivo listo para enviar");
    m_dropLabel->setStyleSheet("color: #3fb950; font-size: 13px;");
    m_uploadBtn->setEnabled(true);
    m_clearBtn->setEnabled(true);
    m_previewCard->setVisible(false);
    m_previewInfoLabel->setText("La vista previa automática comenzará cuando la transferencia llegue al 100%.");
    m_playBtn->setText("Reproducir video cargado");
    m_playBtn->setEnabled(true);
    m_mediaPlayer->setSource(QUrl::fromLocalFile(path));
}

void VideoUploadWidget::setUploadState(Carlens::UploadStatus status)
{
    m_uploadStatus = status;
    bool loading   = (status == Carlens::UploadStatus::Uploading ||
                      status == Carlens::UploadStatus::Processing);

    m_uploadBtn->setEnabled(!loading && !m_selectedFilePath.isEmpty());
    m_selectBtn->setEnabled(!loading);
    m_progressBar->setVisible(loading || status == Carlens::UploadStatus::Completed);
    m_percentLabel->setVisible(loading || status == Carlens::UploadStatus::Completed);
}

void VideoUploadWidget::playUploadedVideo()
{
    if (m_selectedFilePath.isEmpty() || !QFileInfo::exists(m_selectedFilePath)) {
        emit errorMessage("No se encontró el video cargado para reproducir.");
        return;
    }

    m_previewCard->setVisible(true);
    const QUrl sourceUrl = QUrl::fromLocalFile(m_selectedFilePath);
    if (m_mediaPlayer->source() != sourceUrl)
        m_mediaPlayer->setSource(sourceUrl);
    m_mediaPlayer->stop();
    m_mediaPlayer->setPosition(0);
    m_mediaPlayer->play();
    m_previewStartedForCurrentUpload = true;
    m_playBtn->setEnabled(true);
}

void VideoUploadWidget::stopPreviewPlayback()
{
    if (m_mediaPlayer->playbackState() != QMediaPlayer::StoppedState)
        m_mediaPlayer->stop();
}

void VideoUploadWidget::reset()
{
    stopPreviewPlayback();
    m_mediaPlayer->setSource(QUrl());
    m_previewStartedForCurrentUpload = false;
    m_selectedFilePath.clear();
    m_fileInfoLabel->setText("Ningún archivo seleccionado");
    m_fileSizeLabel->clear();
    m_dropLabel->setText("Arrastre un video aquí\no haga clic para seleccionar");
    m_dropLabel->setStyleSheet("color: #8b949e; font-size: 13px;");
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_percentLabel->setVisible(false);
    m_uploadBtn->setEnabled(false);
    m_clearBtn->setEnabled(false);
    m_resultCard->setVisible(false);
    m_playBtn->setVisible(false);
    m_playBtn->setText("Reproducir video cargado");
    m_playBtn->setEnabled(true);
    m_previewCard->setVisible(false);
    m_previewInfoLabel->clear();
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
    // Restore default drop-zone style
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
