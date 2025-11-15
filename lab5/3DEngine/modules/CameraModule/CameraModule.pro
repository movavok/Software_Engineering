TEMPLATE = lib
TARGET = CameraModule
CONFIG += dll c++17
QT += core
INCLUDEPATH += $$PWD/include $$PWD/../../core/include
HEADERS += include/CameraController.h
SOURCES += src/CameraController.cpp
CONFIG(debug, debug|release) {
	LIBS += -L$$OUT_PWD/../../core/debug -lCore
} else {
	LIBS += -L$$OUT_PWD/../../core/release -lCore
}
