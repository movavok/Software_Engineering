TEMPLATE = subdirs
CONFIG += c++17

core.subdir = core
modules_RenderModule.subdir = modules/RenderModule
modules_ModelManager.subdir = modules/ModelManager
modules_CameraModule.subdir = modules/CameraModule
modules_LightModule.subdir = modules/LightModule
app.subdir = app

modules_RenderModule.depends = core
modules_ModelManager.depends = core
modules_CameraModule.depends = core
modules_LightModule.depends = core
app.depends = core modules_RenderModule modules_ModelManager modules_CameraModule modules_LightModule

SUBDIRS += core \
           modules_RenderModule \
           modules_ModelManager \
           modules_CameraModule \
           modules_LightModule \
           app
