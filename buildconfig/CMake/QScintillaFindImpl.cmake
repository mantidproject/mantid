# Implementation of searching for QScintilla. It contains a function
# parameterized by the major version of Qt to be linked against.
#
# The function creates an imported target prefixed in a similar manner
# to the Qt4/Qt5 targets.
function (find_qscintilla qt_version)
  if(qt_version EQUAL 4)
    if ( NOT Qt4_FOUND)
      message ( FATAL_ERROR "find_package ( Qt4 ...) must be called first" )
    endif()
    set ( _qsci_lib_names
      qscintilla2
      libqscintilla2
      qscintilla2_qt4
    )
    set ( _qsci_lib_paths
      ${QT_LIBRARY_DIR}
    )
    set ( _qsci_include_paths
      ${QT_INCLUDE_DIR}
    )
  else()
    if ( NOT Qt5_FOUND)
      message ( FATAL_ERROR "find_package ( Qt5 ...) must be called first" )
    endif()
    set ( _qsci_lib_names
      qscintilla2-qt5
      qscintilla2_qt5
      libqt5scintilla2
      libqscintilla2-qt5
      qt5scintilla2
      libqscintilla2-qt5.dylib
    )
    set ( _qsci_include_paths
      ${Qt5Core_INCLUDE_DIRS}
    )
  endif()

  set ( _include_var QSCINTILLA_QT${qt_version}_INCLUDE_DIR )
  find_path ( ${_include_var}
      NAMES Qsci/qsciglobal.h
      PATHS ${_qsci_include_paths}
      NO_DEFAULT_PATH
  )
set ( _library_var QSCINTILLA_QT${qt_version}_LIBRARY )
  find_library ( ${_library_var}
    NAMES ${_qsci_lib_names}
    PATHS ${_qsci_lib_paths}
  )

  if ( ${_include_var} AND ${_library_var} )
    if ( NOT QScintillaQt${qt_version}_FIND_QUIETLY )
      message ( STATUS "Found QScintilla2 linked against Qt${qt_version}: ${${_library_var}}")
    endif()
    set ( _target_name Qt${qt_version}::Qscintilla )
    add_library ( ${_target_name} UNKNOWN IMPORTED )
    set_target_properties ( ${_target_name} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${${_include_var}}
      IMPORTED_LOCATION ${${_library_var}}
    )
  else ()
    if ( QScintillaQt${qt_version}_FIND_REQUIRED )
      message ( FATAL_ERROR "Failed to find Qscintilla linked against Qt${qt_version}" )
    elseif ( QScintillaQt${qt_version}_FIND_QUIETLY )
      message( WARNING "Failed to find Qscintilla linked against Qt${qt_version}" )
    endif()
  endif ()

  mark_as_advanced (${_include_var} ${_library_var})
#endif()

endfunction()
