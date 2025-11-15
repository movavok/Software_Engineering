TEMPLATE = lib
TARGET = LightModule
CONFIG += dll c++17
QT += core
INCLUDEPATH += $$PWD/include $$PWD/../../core/include
HEADERS += include/LightManager.h
SOURCES += src/LightManager.cpp
CONFIG(debug, debug|release) {
	LIBS += -L$$OUT_PWD/../../core/debug -lCore
} else {
	LIBS += -L$$OUT_PWD/../../core/release -lCore
}
