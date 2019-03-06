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
      libqscintilla2_qt4.dylib
    )
    set ( _qsci_lib_names_debug
      qscintilla2d
    )
    if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
      list ( APPEND _qsci_include_paths /usr/local/opt/qscintilla2qt4/include )
      list ( APPEND _qsci_lib_paths /usr/local/opt/qscintilla2qt4/lib )
    else ()
      set ( _qsci_lib_paths
        ${QT_LIBRARY_DIR}
      )
      set ( _qsci_include_paths
        ${QT_INCLUDE_DIR}
      )
    endif ()
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
      libqscintilla2_qt5.dylib
      qscintilla2
      libqscintilla2
    )
    set ( _qsci_lib_names_debug
      qscintilla2_qt5d
    )
    set ( _qsci_include_paths
      ${Qt5Core_INCLUDE_DIRS}
    )
    if ( MSVC )
      set ( _qsci_lib_paths
        ${THIRD_PARTY_DIR}/lib/qt5/lib
      )
    elseif ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
      list ( APPEND _qsci_include_paths /usr/local/opt/qscintilla2/include )
      list ( APPEND _qsci_lib_paths /usr/local/opt/qscintilla2/lib )
    endif ()
  endif()

  set ( _include_var QSCINTILLA_QT${qt_version}_INCLUDE_DIR )
  find_path ( ${_include_var}
      NAMES Qsci/qsciglobal.h
      PATHS ${_qsci_include_paths}
      NO_DEFAULT_PATH
  )
  set ( _library_var QSCINTILLA_QT${qt_version}_LIBRARY )
  if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    set ( _default_path_opt NO_DEFAULT_PATH )
  endif()
  find_library ( ${_library_var}
    NAMES ${_qsci_lib_names}
    PATHS ${_qsci_lib_paths}
    ${_default_path_opt}
  )
  set ( _library_var_debug QSCINTILLA_QT${qt_version}_LIBRARY_DEBUG )
  find_library ( ${_library_var_debug}
    NAMES ${_qsci_lib_names_debug}
    PATHS ${_qsci_lib_paths}
    ${_default_path_opt}
  )

  if ( ${_include_var} AND ${_library_var} )
    if ( NOT QScintillaQt${qt_version}_FIND_QUIETLY )
      message ( STATUS "Found QScintilla2 linked against Qt${qt_version}: ${${_library_var}}")
    endif()

    set ( _target_name Qt${qt_version}::Qscintilla )
    add_library ( ${_target_name} SHARED IMPORTED )
    if ( WIN32 )
      set ( _lib_import_var IMPORTED_IMPLIB )
    else ()
      set ( _lib_import_var IMPORTED_LOCATION )
    endif ()
    set_property ( TARGET ${_target_name} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE )
    set_target_properties ( ${_target_name} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${${_include_var}}
      ${_lib_import_var}_RELEASE "${${_library_var}}"
    )
    if ( ${_library_var_debug} )
      set_property ( TARGET ${_target_name} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties ( ${_target_name} PROPERTIES
        ${_lib_import_var}_DEBUG "${${_library_var_debug}}"
      )
    endif ()
  else ()
    if ( QScintillaQt${qt_version}_FIND_REQUIRED )
      message ( FATAL_ERROR "Failed to find Qscintilla linked against Qt${qt_version}" )
    elseif ( QScintillaQt${qt_version}_FIND_QUIETLY )
      message( WARNING "Failed to find Qscintilla linked against Qt${qt_version}" )
    endif()
  endif ()

  mark_as_advanced (${_include_var} ${_library_var} ${_library_var_debug})

endfunction()
