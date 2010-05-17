TEMPLATE=lib
CONFIG += shared
CONFIG += release_and_debug

INCLUDEPATH += src
DEPENDPATH += src

SOURCES += src/qtpropertybrowser.cpp \
        src/qtpropertymanager.cpp \
        src/qteditorfactory.cpp \
        src/qtvariantproperty.cpp \
        src/qttreepropertybrowser.cpp \
        src/qtbuttonpropertybrowser.cpp \
        src/qtgroupboxpropertybrowser.cpp \
        src/qtpropertybrowserutils.cpp \
        src/FilenameEditorFactory.cpp
HEADERS += src/qtpropertybrowser.h \
        src/qtpropertymanager.h \
        src/qteditorfactory.h \
        src/qtvariantproperty.h \
        src/qttreepropertybrowser.h \
        src/qtbuttonpropertybrowser.h \
        src/qtgroupboxpropertybrowser.h \
        src/qtpropertybrowserutils_p.h \
        src/FilenameEditorFactory.h 
RESOURCES += src/qtpropertybrowser.qrc

win32 {
    DEFINES += QT_QTPROPERTYBROWSER_EXPORT
}

win32:build_pass:CONFIG(debug, debug|release) {
  # Put alongside Mantid libraries
  DESTDIR = ..\..\Mantid\debug
  TARGET = QtPropertyBrowserd
} else {
  # Put in local output directory
  DESTDIR = lib
  TARGET = QtPropertyBrowser
}

CONFIG += release
