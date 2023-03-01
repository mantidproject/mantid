# Implementation of searching for QScintilla. It contains a function parameterized by the major version of Qt to be
# linked against.
#
# The function creates an imported target prefixed in a similar manner to the Qt5 targets.
function(find_qscintilla qt_version)
  if(NOT Qt5_FOUND)
    message(FATAL_ERROR "find_package ( Qt5 ...) must be called first")
  endif()
  set(_qsci_lib_names
      qscintilla2-qt5
      qscintilla2_qt5
      libqt5scintilla2
      libqscintilla2-qt5
      qt5scintilla2
      libqscintilla2_qt5
      qscintilla2
      libqscintilla2
  )
  set(_qsci_lib_names_debug qscintilla2_qt5d)
  set(_qsci_include_paths ${Qt5Core_INCLUDE_DIRS})

  set(_include_var QSCINTILLA_QT${qt_version}_INCLUDE_DIR)
  find_path(
    ${_include_var}
    NAMES Qsci/qsciglobal.h
    HINTS ${_qsci_include_paths} PATH_PREFIXES include/qt include
  )

  set(_library_var QSCINTILLA_QT${qt_version}_LIBRARY)
  find_library(${_library_var} NAMES ${_qsci_lib_names})
  set(_library_var_debug QSCINTILLA_QT${qt_version}_LIBRARY_DEBUG)
  find_library(${_library_var_debug} NAMES ${_qsci_lib_names_debug})

  if(${_include_var} AND ${_library_var})
    if(NOT QScintillaQt${qt_version}_FIND_QUIETLY)
      message(STATUS "Found QScintilla2 linked against Qt${qt_version}: ${${_library_var}}")
    endif()

    set(_target_name Qt${qt_version}::Qscintilla)
    add_library(${_target_name} SHARED IMPORTED)
    if(WIN32)
      set(_lib_import_var IMPORTED_IMPLIB)
    else()
      set(_lib_import_var IMPORTED_LOCATION)
    endif()
    set_property(
      TARGET ${_target_name}
      APPEND
      PROPERTY IMPORTED_CONFIGURATIONS RELEASE
    )
    set_target_properties(
      ${_target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${${_include_var}} ${_lib_import_var}_RELEASE
                                                                                  "${${_library_var}}"
    )
    if(${_library_var_debug})
      set_property(
        TARGET ${_target_name}
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS DEBUG
      )
      set_target_properties(${_target_name} PROPERTIES ${_lib_import_var}_DEBUG "${${_library_var_debug}}")
    endif()
  endif()

  mark_as_advanced(${_include_var} ${_library_var} ${_library_var_debug})

endfunction()
