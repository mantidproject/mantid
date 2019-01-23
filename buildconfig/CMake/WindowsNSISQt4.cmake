set ( QT4_DIST_DLLS
    phonon4.dll
    Qt3Support4.dll
    QtCLucene4.dll
    QtCore4.dll
    QtDeclarative4.dll
    QtDesigner4.dll
    QtDesignerComponents4.dll
    QtGui4.dll
    QtHelp4.dll
    QtMultimedia4.dll
    QtNetwork4.dll
    QtOpenGL4.dll
    QtScript4.dll
    QtScriptTools4.dll
    QtSql4.dll
    QtSvg4.dll
    QtTest4.dll
    QtWebKit4.dll
    QtXml4.dll
    QtXmlPatterns4.dll
    qscintilla2.dll )

set ( QT4_INSTALL_PREFIX ${THIRD_PARTY_DIR}/lib/qt4 )
foreach( DLL ${QT4_DIST_DLLS} )
  install ( FILES ${QT4_INSTALL_PREFIX}/lib/${DLL} DESTINATION bin )
endforeach()

###########################################################################
# Qt Plugins + qt.conf file
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/WinInstaller/qt.conf DESTINATION bin )

set ( QT4_PLUGINS_IMAGEFORMAT qgif4.dll qico4.dll qjpeg4.dll qmng4.dll qsvg4.dll qtga4.dll qtiff4.dll )

set ( QT4_PLUGIN_DIR ${QT4_INSTALL_PREFIX}/plugins )
foreach( DLL ${QT4_PLUGINS_IMAGEFORMAT} )
  install ( FILES ${QT4_PLUGIN_DIR}/imageformats/${DLL} DESTINATION plugins/qt4/imageformats )
endforeach()

install ( FILES ${QT4_PLUGIN_DIR}/sqldrivers/qsqlite4.dll DESTINATION plugins/qt4/sqldrivers )