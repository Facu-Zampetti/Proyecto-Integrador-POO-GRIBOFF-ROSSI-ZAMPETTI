QT += widgets

TEMPLATE = app
TARGET = CarlensQtRunner

CONFIG += c++17 release

MOC_DIR = $$OUT_PWD/moc
OBJECTS_DIR = $$OUT_PWD/obj
RCC_DIR = $$OUT_PWD/rcc
UI_DIR = $$OUT_PWD/ui

win32:DESTDIR = $$OUT_PWD/bin

SOURCES += \
    CaptureManager.cpp \
    CompositeSnapshotPersistenceBackend.cpp \
    FileSystemSnapshotPersistenceBackend.cpp \
    FramePayloadParser.cpp \
    main.cpp \
    MainWindow.cpp \
    RemoteSnapshotPersistenceBackend.cpp \
    VideoWidget.cpp \
    YoloProcessController.cpp

HEADERS += \
    CaptureManager.h \
    CapturedVehicleSnapshot.h \
    CompositeSnapshotPersistenceBackend.h \
    DetectionTypes.h \
    FileSystemSnapshotPersistenceBackend.h \
    FramePayload.h \
    FramePayloadParser.h \
    MainWindow.h \
    RemoteSnapshotPersistenceBackend.h \
    SnapshotPersistenceBackend.h \
    VideoWidget.h \
    YoloProcessController.h