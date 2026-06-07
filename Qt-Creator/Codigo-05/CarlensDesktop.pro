QT += core gui widgets network sql multimedia multimediawidgets concurrent svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

TARGET   = Carlens
TEMPLATE = app

INCLUDEPATH += src/

FORMS += \
    src/widgets/LoginWidget.ui

HEADERS += \
    src/utils/AppEnums.h          \
    src/base/IManager.h           \
    src/base/BaseWidget.h         \
    src/base/Singleton.h          \
    src/models/UserModel.h        \
    src/models/VehicleModel.h     \
    src/models/AnalysisModel.h    \
    src/models/SessionModel.h     \
    src/services/AuthService.h    \
    src/services/RemoteAuthBackend.h \
    src/services/RemoteHistoryBackend.h \
    src/services/adminDB.h\
    src/managers/SessionManager.h \
    src/managers/ConfigManager.h  \
    src/managers/UserManager.h    \
    src/managers/VideoManager.h   \
    src/managers/HistoryManager.h \
    src/utils/CryptoHelper.h      \
    src/utils/StyleManager.h      \
    src/widgets/MainWindow.h      \
    src/widgets/LoginWidget.h     \
    src/widgets/RegisterWidget.h  \
    src/widgets/DashboardWidget.h \
    src/widgets/SidebarWidget.h   \
    src/widgets/YoloPipelineWidget.h    \
    src/widgets/VideoUploadWidget.h     \
    src/widgets/RtspWidget.h            \
    src/widgets/HistoryWidget.h         \
    src/widgets/VehicleAnalysisWidget.h \
    src/widgets/VehicleCardWidget.h     \
    src/widgets/StatusBarWidget.h       \
    src/widgets/SettingsWidget.h        \
    src/yolo/DetectionTypes.h           \
    src/yolo/FramePayload.h             \
    src/yolo/FramePayloadParser.h       \
    src/yolo/YoloProcessController.h    \
    src/yolo/VideoWidget.h              \
    src/yolo/CapturedVehicleSnapshot.h  \
    src/yolo/SnapshotPersistenceBackend.h            \
    src/yolo/FileSystemSnapshotPersistenceBackend.h  \
    src/yolo/CompositeSnapshotPersistenceBackend.h   \
    src/yolo/RemoteSnapshotPersistenceBackend.h      \
    src/yolo/CaptureManager.h           \
    src/yolo/GeminiAnalysisService.h

SOURCES += \
    main.cpp                             \
    src/base/BaseWidget.cpp              \
    src/models/UserModel.cpp             \
    src/models/VehicleModel.cpp          \
    src/models/AnalysisModel.cpp         \
    src/models/SessionModel.cpp          \
    src/services/AuthService.cpp         \
    src/services/RemoteAuthBackend.cpp   \
    src/services/RemoteHistoryBackend.cpp \
    src/services/adminDB.cpp     \
    src/managers/SessionManager.cpp      \
    src/managers/ConfigManager.cpp       \
    src/managers/UserManager.cpp         \
    src/managers/VideoManager.cpp        \
    src/managers/HistoryManager.cpp      \
    src/utils/CryptoHelper.cpp           \
    src/utils/StyleManager.cpp           \
    src/widgets/MainWindow.cpp           \
    src/widgets/LoginWidget.cpp          \
    src/widgets/RegisterWidget.cpp       \
    src/widgets/DashboardWidget.cpp      \
    src/widgets/SidebarWidget.cpp        \
    src/widgets/YoloPipelineWidget.cpp   \
    src/widgets/VideoUploadWidget.cpp    \
    src/widgets/RtspWidget.cpp           \
    src/widgets/HistoryWidget.cpp        \
    src/widgets/VehicleAnalysisWidget.cpp\
    src/widgets/VehicleCardWidget.cpp    \
    src/widgets/StatusBarWidget.cpp      \
    src/widgets/SettingsWidget.cpp       \
    src/yolo/FramePayloadParser.cpp      \
    src/yolo/YoloProcessController.cpp   \
    src/yolo/VideoWidget.cpp             \
    src/yolo/FileSystemSnapshotPersistenceBackend.cpp  \
    src/yolo/CompositeSnapshotPersistenceBackend.cpp   \
    src/yolo/RemoteSnapshotPersistenceBackend.cpp      \
    src/yolo/CaptureManager.cpp          \
    src/yolo/GeminiAnalysisService.cpp

RESOURCES += resources/resources.qrc

# Windows-specific: console for debug builds
CONFIG(debug, debug|release) {
    CONFIG += console
}
