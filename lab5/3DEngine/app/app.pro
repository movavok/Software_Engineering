TEMPLATE = app
TARGET = 3DEngine
CONFIG += c++17
QT += core gui widgets openglwidgets opengl
INCLUDEPATH += $$PWD/../core/include \
               $$PWD/../modules/RenderModule/include \
               $$PWD/../modules/ModelManager/include \
               $$PWD/../modules/CameraModule/include \
               $$PWD/../modules/LightModule/include
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    widgets/SceneViewWidget.cpp
HEADERS += \
    mainwindow.h \
    widgets/SceneViewWidget.h
FORMS += \
    mainwindow.ui
CONFIG(debug, debug|release) {
    LIBS += -L$$OUT_PWD/../core/debug -lCore \
            -L$$OUT_PWD/../modules/RenderModule/debug -lRenderModule \
            -L$$OUT_PWD/../modules/ModelManager/debug -lModelManager \
            -L$$OUT_PWD/../modules/CameraModule/debug -lCameraModule \
            -L$$OUT_PWD/../modules/LightModule/debug -lLightModule
} else {
    LIBS += -L$$OUT_PWD/../core/release -lCore \
            -L$$OUT_PWD/../modules/RenderModule/release -lRenderModule \
            -L$$OUT_PWD/../modules/ModelManager/release -lModelManager \
            -L$$OUT_PWD/../modules/CameraModule/release -lCameraModule \
            -L$$OUT_PWD/../modules/LightModule/release -lLightModule
}

RESOURCES += \
    resources.qrc

RC_ICONS = icons/app_icon.ico
