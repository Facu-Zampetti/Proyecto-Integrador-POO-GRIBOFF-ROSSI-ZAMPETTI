#ifndef VIDEOUPLOADWIDGET_H
#define VIDEOUPLOADWIDGET_H

#include <QLabel>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "base/BaseWidget.h"
#include "models/SessionModel.h"
#include "utils/AppEnums.h"

/**
 * @file VideoUploadWidget.h
 * @brief Módulo de carga de video MP4. Hereda de BaseWidget.
 *
 * Funcionalidades:
 *   - Selección de archivo con QFileDialog.
 *   - Preview del nombre y metadatos del archivo.
 *   - Progreso de carga en tiempo real (QProgressBar).
 *   - Comunicación con VideoManager.
 *   - Muestra resultado de la sesión al completar.
 */
class VideoUploadWidget : public BaseWidget {
    Q_OBJECT

public:
    explicit VideoUploadWidget(QWidget* parent = nullptr);
    QString widgetName() const override { return "VideoUploadWidget"; }
    void    reset() override;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void successMessage(const QString& msg);

protected:
    void setupUI()        override;
    void connectSignals() override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event)       override;

private slots:
    void onSelectFileClicked();
    void onUploadClicked();
    void onPlayUploadedVideoClicked();
    void onUploadStarted(const QString& filename);
    void onProgressChanged(int percent);
    void onUploadCompleted(const SessionModel& session);
    void onUploadFailed(const QString& reason);

private:
    void setUploadState(Carlens::UploadStatus status);
    void playUploadedVideo();
    void showFileInfo(const QString& path);
    void stopPreviewPlayback();

    // ── UI ─────────────────────────────────────────────────────────────
    QWidget*     m_dropZone      = nullptr;
    QLabel*      m_dropLabel     = nullptr;
    QLabel*      m_fileInfoLabel = nullptr;
    QLabel*      m_fileSizeLabel = nullptr;
    QProgressBar* m_progressBar  = nullptr;
    QLabel*      m_percentLabel  = nullptr;
    QPushButton* m_selectBtn     = nullptr;
    QPushButton* m_uploadBtn     = nullptr;
    QPushButton* m_clearBtn      = nullptr;
    QPushButton* m_playBtn       = nullptr;
    QWidget*     m_resultCard    = nullptr;
    QLabel*      m_resultLabel   = nullptr;
    QScrollArea* m_scrollArea    = nullptr;
    QWidget*     m_scrollContent = nullptr;
    QWidget*     m_previewCard   = nullptr;
    QVideoWidget* m_previewVideo = nullptr;
    QLabel*      m_previewInfoLabel = nullptr;

    QString m_selectedFilePath;
    Carlens::UploadStatus m_uploadStatus = Carlens::UploadStatus::Idle;
    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    bool m_previewStartedForCurrentUpload = false;
};

#endif // VIDEOUPLOADWIDGET_H
