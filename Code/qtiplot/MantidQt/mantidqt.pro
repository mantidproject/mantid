#--------------------------------
# There are two libraries here:
#   - MantidQtAPI
#   - MantidQtCustomDialogs
#--------------------------------
include(mantidqt.pri)

system(mkdir $$MANTIDQTINCLUDES)

TEMPLATE = subdirs
SUBDIRS = API CustomDialogs

#----------------------------
# Clean this directory as well
#-----------------------------
unix {
QMAKE_CLEAN += ./lib/* $$MANTIDQTINCLUDES/MantidQtAPI/*.h \
               $$MANTIDQTINCLUDES/MantidQtCustomDialogs/*.h
}
win32 {
QMAKE_CLEAN += lib\*.lib lib\*.dll lib\*.dll.manifest $$MANTIDQTINCLUDES\MantidQtAPI\*.h \
               $$MANTIDQTINCLUDES\MantidQtCustomDialogs\*.h qtbuild\MantidQt\sip*
}