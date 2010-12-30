TEMPLATE=lib
CONFIG += shared
CONFIG += debug_and_release

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
        src/StringDialogEditorFactory.cpp \
        src/DoubleEditorFactory.cpp
HEADERS += src/qtpropertybrowser.h \
        src/qtpropertymanager.h \
        src/qteditorfactory.h \
        src/qtvariantproperty.h \
        src/qttreepropertybrowser.h \
        src/qtbuttonpropertybrowser.h \
        src/qtgroupboxpropertybrowser.h \
        src/qtpropertybrowserutils_p.h \
        src/StringDialogEditorFactory.h \
        src/DoubleEditorFactory.h
RESOURCES += src/qtpropertybrowser.qrc

win32 {
    DEFINES += QT_QTPROPERTYBROWSER_EXPORT
}

# Put alongside Mantid libraries
build_pass:CONFIG(debug, debug|release) {
  DESTDIR = ..\Framework\debug
  TARGET = QtPropertyBrowserd
} else {
  # Put in local output directory
  DESTDIR = ..\Framework\release
  TARGET = QtPropertyBrowser
}

CONFIG += release
