include(mantidqt.pri)

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = API MantidWidgets CustomDialogs CustomInterfaces DesignerPlugins

#----------------------------
# Clean this directory as well
#-----------------------------
unix {
QMAKE_CLEAN += ./lib/* $$MANTIDQTINCLUDES/MantidQtAPI/*.h \
               $$MANTIDQTINCLUDES/MantidQtCustomDialogs/*.h \
               $$MANTIDQTINCLUDES/MantidQtCustomInterfaces/*.h \
               $$MANTIDQTINCLUDES/MantidQtMantidWidgets/*.h
}
win32 {
QMAKE_CLEAN += $$DESTDIR\*.lib $$DESTDIR\*.dll $$DESTDIR\*.dll.manifest \
			   "$$MANTIDQTINCLUDES"\MantidQtAPI\*.h" \
         "$$MANTIDQTINCLUDES"\MantidQtCustomDialogs\*.h" \
			   "$$MANTIDQTINCLUDES"\MantidQtCustomInterfaces\*.h" \
			   "$$MANTIDQTINCLUDES"\MantidQtCustomWidgets\*.h" \
			   qtbuild\MantidQt\sip*
}
