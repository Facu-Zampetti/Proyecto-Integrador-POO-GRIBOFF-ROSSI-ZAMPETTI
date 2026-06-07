#ifndef VIDEOUPLOADWIDGET_H
#define VIDEOUPLOADWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "widgets/YoloPipelineWidget.h"

/**
 * @file VideoUploadWidget.h
 * @brief Módulo "Cargar Video": detección YOLO local sobre un archivo. Hereda de YoloPipelineWidget.
 *
 * Añade la UI de selección de archivo (drag & drop + QFileDialog) y los botones
 * Analizar/Cancelar. El pipeline YOLO completo (captura → sincronización → Gemini)
 * vive en la clase base.
 *
 * Implementa los hooks del Template Method:
 *   - statusToString(): "Inactivo / Iniciando… / Procesando — En vivo / Error"
 *   - updateControls(): habilita los botones según el estado del procesamiento.
 */
class VideoUploadWidget : public YoloPipelineWidget {
    Q_OBJECT

public:
    explicit VideoUploadWidget(QWidget* parent = nullptr);
    ~VideoUploadWidget() override;

    QString widgetName() const override { return "VideoUploadWidget"; }
    void    reset() override;

protected:
    void setupUI()        override;
    void connectSignals() override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event)       override;

    QString statusToString(Carlens::StreamStatus status) const override;
    void    updateControls(Carlens::StreamStatus status) override;

private slots:
    void onSelectFileClicked();
    void onAnalyzeClicked();
    void onCancelClicked();

private:
    void showFileInfo(const QString& path);

    // ── UI específica de este widget ──────────────────────────────────────
    QScrollArea* m_scrollArea    = nullptr;
    QWidget*     m_scrollContent = nullptr;
    QWidget*     m_dropZone      = nullptr;
    QLabel*      m_dropLabel     = nullptr;
    QLabel*      m_fileInfoLabel = nullptr;
    QLabel*      m_fileSizeLabel = nullptr;
    QPushButton* m_selectBtn     = nullptr;
    QPushButton* m_analyzeBtn    = nullptr;
    QPushButton* m_cancelBtn     = nullptr;
    QPushButton* m_clearBtn      = nullptr;

    QString m_selectedFilePath;
};

#endif // VIDEOUPLOADWIDGET_H
