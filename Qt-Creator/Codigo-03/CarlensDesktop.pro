QT += core gui widgets network sql multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += QT_DEPRECATED_WARNINGS

TARGET   = Carlens
TEMPLATE = app

INCLUDEPATH += src/

HEADERS += \
    src/utils/AppEnums.h          \
    src/base/IManager.h           \
    src/base/AbstractApiService.h \
    src/base/BaseWidget.h         \
    src/models/UserModel.h        \
    src/models/VehicleModel.h     \
    src/models/AnalysisModel.h    \
    src/models/SessionModel.h     \
    src/services/ApiClient.h      \
    src/services/AuthService.h    \
    src/services/VideoService.h   \
    src/services/AnalysisService.h\
    src/services/DatabaseService.h\
    src/managers/SessionManager.h \
    src/managers/ConfigManager.h  \
    src/managers/UserManager.h    \
    src/managers/VideoManager.h   \
    src/managers/RtspManager.h    \
    src/managers/HistoryManager.h \
    src/utils/CryptoHelper.h      \
    src/utils/StyleManager.h      \
    src/widgets/MainWindow.h      \
    src/widgets/LoginWidget.h     \
    src/widgets/RegisterWidget.h  \
    src/widgets/DashboardWidget.h \
    src/widgets/SidebarWidget.h   \
    src/widgets/VideoUploadWidget.h     \
    src/widgets/RtspWidget.h            \
    src/widgets/HistoryWidget.h         \
    src/widgets/VehicleAnalysisWidget.h \
    src/widgets/VehicleCardWidget.h     \
    src/widgets/StatusBarWidget.h

SOURCES += \
    main.cpp                             \
    src/base/AbstractApiService.cpp      \
    src/base/BaseWidget.cpp              \
    src/models/UserModel.cpp             \
    src/models/VehicleModel.cpp          \
    src/models/AnalysisModel.cpp         \
    src/models/SessionModel.cpp          \
    src/services/ApiClient.cpp           \
    src/services/AuthService.cpp         \
    src/services/VideoService.cpp        \
    src/services/AnalysisService.cpp     \
    src/services/DatabaseService.cpp     \
    src/managers/SessionManager.cpp      \
    src/managers/ConfigManager.cpp       \
    src/managers/UserManager.cpp         \
    src/managers/VideoManager.cpp        \
    src/managers/RtspManager.cpp         \
    src/managers/HistoryManager.cpp      \
    src/utils/CryptoHelper.cpp           \
    src/utils/StyleManager.cpp           \
    src/widgets/MainWindow.cpp           \
    src/widgets/LoginWidget.cpp          \
    src/widgets/RegisterWidget.cpp       \
    src/widgets/DashboardWidget.cpp      \
    src/widgets/SidebarWidget.cpp        \
    src/widgets/VideoUploadWidget.cpp    \
    src/widgets/RtspWidget.cpp           \
    src/widgets/HistoryWidget.cpp        \
    src/widgets/VehicleAnalysisWidget.cpp\
    src/widgets/VehicleCardWidget.cpp    \
    src/widgets/StatusBarWidget.cpp

RESOURCES += resources/resources.qrc

# Windows-specific: console for debug builds
CONFIG(debug, debug|release) {
    CONFIG += console
}
