TEMPLATE = lib
TARGET = RenderModule
CONFIG += dll c++17
QT += core gui widgets opengl openglwidgets
INCLUDEPATH += $$PWD/include \
               $$PWD/../../core/include \
               $$PWD/../../third_party/assimp \
               $$PWD/../../third_party/stb_image
DEFINES += RENDERMODULE_LIBRARY
HEADERS += include/Renderer.h
SOURCES += src/Renderer.cpp
# Link against built core output (two levels up to build root)
CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../../core/debug -lCore
} else {
    LIBS += -L$$OUT_PWD/../../core/release -lCore
}
