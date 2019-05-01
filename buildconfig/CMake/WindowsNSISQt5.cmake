###########################################################################
# Qt 5 DLLs
###########################################################################
set ( QT5_DIST_DLLS
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Widgets.dll
    Qt5PrintSupport.dll
    Qt5Svg.dll
    Qt5OpenGL.dll
    Qt5Help.dll
    Qt5Network.dll
    Qt5Positioning.dll
    Qt5Qml.dll
    Qt5Quick.dll
    Qt5QuickWidgets.dll
    Qt5Sql.dll
    Qt5WebChannel.dll
    Qt5WebEngine.dll
    Qt5WebEngineCore.dll
    Qt5WebEngineWidgets.dll
)

set ( QT5_INSTALL_PREFIX ${THIRD_PARTY_DIR}/lib/qt5 )
foreach( DLL ${QT5_DIST_DLLS} )
  install ( FILES ${QT5_INSTALL_PREFIX}/bin/${DLL} DESTINATION bin )
endforeach()

###########################################################################
# Qt Plugins
###########################################################################
set ( QT5_PLUGINS_IMAGEFORMAT qgif.dll qico.dll qjpeg.dll qsvg.dll qtga.dll qtiff.dll )
set ( QT5_PLUGIN_DIR ${QT5_INSTALL_PREFIX}/plugins )
foreach( DLL ${QT5_PLUGINS_IMAGEFORMAT} )
  install ( FILES ${QT5_PLUGIN_DIR}/imageformats/${DLL} DESTINATION plugins/qt5/imageformats )
endforeach()

install ( FILES ${QT5_PLUGIN_DIR}/platforms/qwindows.dll DESTINATION plugins/qt5/platforms )
install ( FILES ${QT5_PLUGIN_DIR}/sqldrivers/qsqlite.dll DESTINATION plugins/qt5/sqldrivers )
install ( FILES ${QT5_PLUGIN_DIR}/styles/qwindowsvistastyle.dll DESTINATION plugins/qt5/styles )

install ( FILES ${QT5_INSTALL_PREFIX}/lib/qscintilla2_qt5.dll DESTINATION bin )

###########################################################################
# Qt Resource Files for QtWebEngine
###########################################################################
install ( FILES ${QT5_INSTALL_PREFIX}/bin/QtWebEngineProcess.exe DESTINATION lib/qt5/bin )
install ( DIRECTORY ${QT5_INSTALL_PREFIX}/resources DESTINATION lib/qt5 )
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/qt/applications/workbench/resources.py.install
          DESTINATION bin/workbench/app RENAME resources.py)
