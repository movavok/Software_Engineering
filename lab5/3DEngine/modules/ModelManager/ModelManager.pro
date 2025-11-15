TEMPLATE = lib
TARGET = ModelManager
CONFIG += dll c++17
QT += core
INCLUDEPATH += $$PWD/include \
               $$PWD/../../core/include \
               $$PWD/../../third_party/assimp \
               $$PWD/../../third_party/stb_image
HEADERS += include/ModelManager.h
SOURCES += src/ModelManager.cpp
CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../../core/debug -lCore
} else {
    LIBS += -L$$OUT_PWD/../../core/release -lCore
}
