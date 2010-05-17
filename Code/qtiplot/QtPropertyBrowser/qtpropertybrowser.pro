TEMPLATE=lib
CONFIG += shared debug_and_release

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

build_pass:win32 {
    DEFINES += QT_QTPROPERTYBROWSER_EXPORT
}

build_pass:CONFIG(debug, debug|release) {
  # Put alongside Mantid libraries
  DESTDIR = ../../Mantid/debug
  win32:TARGET = QtPropertyBrowserd
  unix:TARGET=QtPropertyBrowser
} else {
  # Put in local output directory
  DESTDIR = lib
  TARGET = QtPropertyBrowser
}


