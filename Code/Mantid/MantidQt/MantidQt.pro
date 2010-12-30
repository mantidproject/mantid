include(mantidqt.pri)

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = API MantidWidgets CustomDialogs CustomInterfaces DesignerPlugins

#----------------------------
# Clean this directory as well
#-----------------------------
unix {
QMAKE_CLEAN += ./lib/* 
}
win32 {
QMAKE_CLEAN += $$DESTDIR\*.lib $$DESTDIR\*.dll $$DESTDIR\*.dll.manifest \
			   qtbuild\MantidQt\sip*
}
