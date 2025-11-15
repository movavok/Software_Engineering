TEMPLATE = lib
TARGET = Core
CONFIG += staticlib c++17
QT += core
INCLUDEPATH += $$PWD/include
HEADERS += \
    include/Color.h \
    include/Vec3.h \
    include/Camera.h \
    include/Light.h \
    include/Scene.h \
    include/SceneNode.h \
    include/Component.h \
    include/Model.h \
    include/Mesh.h \
    include/Material.h \
    include/Texture.h \

SOURCES += \
    src/Color.cpp \
    src/Camera.cpp \
    src/Light.cpp \
    src/Scene.cpp \
    src/SceneNode.cpp \
    src/Component.cpp \
    src/Model.cpp \
    src/Mesh.cpp \
    src/Material.cpp \
    src/Texture.cpp
