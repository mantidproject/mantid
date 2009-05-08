#--------------------------------
# There are two libraries here:
#   - MantidQtAPI
#   - MantidQtCustomDialogs
#--------------------------------
include(mantidqt.pri)

system(mkdir '"$$MANTIDQTINCLUDES"')

TEMPLATE = subdirs
SUBDIRS = API CustomDialogs CustomInterfaces

#----------------------------
# Clean this directory as well
#-----------------------------
unix {
QMAKE_CLEAN += ./lib/* $$MANTIDQTINCLUDES/MantidQtAPI/*.h \
               $$MANTIDQTINCLUDES/MantidQtCustomDialogs/*.h
}
win32 {
QMAKE_CLEAN += $$DESTDIR\*.lib $$DESTDIR\*.dll $$DESTDIR\*.dll.manifest "$$MANTIDQTINCLUDES"\MantidQtAPI\*.h \
               "$$MANTIDQTINCLUDES"\MantidQtCustomDialogs\*.h "$$MANTIDQTINCLUDES"\MantidQtCustomInterfaces\*.h qtbuild\MantidQt\sip*
}
