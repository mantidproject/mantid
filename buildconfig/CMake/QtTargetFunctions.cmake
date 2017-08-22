# Utility functions to add libraries/executables parametrized by
# the required version of Qt
#
# name: mtd_add_qt_library
# brief: create a library target for both Qt4 & Qt5
# For all options see mtd_add_qt_target
function (mtd_add_qt_library)
  mtd_add_qt_target (LIBRARY QT_VERSION 4 ${ARGN})
  #mtd_add_qt_target (LIBRARY QT_VERSION 5 ${ARGN})
endfunction()

# name: mtd_add_qt_executable
# brief: create an executable target for a given Qt version
# For all options see mtd_add_qt_target
function (mtd_add_qt_executable)
  mtd_add_qt_target (EXECUTABLE QT_VERSION 4 ${ARGN})
  mtd_add_qt_target (EXECUTABLE QT_VERSION 5 ${ARGN})
endfunction()

# Target agnostic function to add either an executable or library linked to Qt
# option: LIBRARY If included define a library target
# option: EXECUTABLE If included define an executable target
# option: NO_SUFFIX If included, no suffix is added to the target name
# keyword: TARGET_NAME The name of the target. The target will have -Qt{QT_VERSION} appended to it.
# keyword: QT_VERSION The major version of Qt to build against
# keyword: SRC .cpp files to include in the target build
# keyword: MOC Header files that are to be parsed by moc
# keyword: NOMOC Additional headers that are not to be passed to moc
# keyword: INCLUDE_DIRS A list of include directories to add to the target
# keyword: LINK_LIBRARIES A list of additional libraries to link to the
#          target that are not dependent on Qt
# keyword: MTD_QT_LINK_LIBRARIES A list of additional libraries to link to the
#          target. It is assumed each was produced with this function and
#          will have the -Qt{QT_VERSION} suffix appended.
function (mtd_add_qt_target)
  set (options LIBRARY EXECUTABLE NO_SUFFIX)
  set (oneValueArgs TARGET_NAME QT_VERSION)
  set (multiValueArgs SRC UI MOC NOMOC INCLUDE_DIRS LINK_LIBRARIES MTD_QT_LINK_LIBRARIES)
  cmake_parse_arguments (PARSED "${options}" "${oneValueArgs}"
                         "${multiValueArgs}" ${ARGN})
  if (${PARSED_LIBRARY} AND ${PARSED_EXECUTABLE})
    message (FATAL_ERROR "Both LIBRARY and EXECUTABLE options specified. Please choose only one.")
  endif()
  set (_base_src_dir ${CMAKE_CURRENT_LIST_DIR})
  set (_binary_dir_on_entry ${CMAKE_CURRENT_BINARY_DIR})
  set (_base_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/qt${PARSED_QT_VERSION})
  set (CMAKE_CURRENT_BINARY_DIR ${_base_binary_dir})
  if (PARSED_QT_VERSION EQUAL 4)
    qt4_wrap_ui (UI_HEADERS ${PARSED_UI})
    qt4_wrap_cpp (MOC_GENERATED ${PARSED_MOC})
    set (_qt_link_libraries Qt4::QtGui)
  elseif (PARSED_QT_VERSION EQUAL 5)
      qt5_wrap_ui (UI_HEADERS ${PARSED_UI})
      qt5_wrap_cpp (MOC_GENERATED ${PARSED_MOC})
      set (_qt_link_libraries Qt5::Widgets)
  else ()
    message (FATAL_ERROR "Unknown Qt version. Please specify only the major version.")
  endif()
  set (CMAKE_CURRENT_BINARY_DIR ${_binary_dir_on_entry})
  set (_target_suffix "Qt${PARSED_QT_VERSION}")
  if (${PARSED_NO_SUFFIX})
    set (_target ${PARSED_TARGET_NAME})
  else()
    set (_target ${PARSED_TARGET_NAME}${_target_suffix})
  endif()
  if (${PARSED_LIBRARY})
    add_library (${_target} ${PARSED_SRC} ${MOC_GENERATED} ${UI_HEADERS} ${PARSED_NOMOC})
  elseif (${PARSED_EXECUTABLE})
    add_executable (${_target} ${PARSED_SRC} ${MOC_GENERATED} ${UI_HEADERS} ${PARSED_NOMOC})
  else ()
    message (FATAL_ERROR "Unknown target type. Options=LIBRARY,EXECUTABLE")
  endif()
  target_include_directories (${_target} PUBLIC ${_base_binary_dir} PUBLIC ${PARSED_INCLUDE_DIRS})
  # Append suffix to libraries created with these functions
  set (_mtd_qt_libs)
  foreach (_lib ${PARSED_MTD_QT_LINK_LIBRARIES})
    list (APPEND _mtd_qt_libs ${_lib}${_target_suffix})
  endforeach ()
  target_link_libraries (${_target} PRIVATE ${_qt_link_libraries}
                         ${PARSED_LINK_LIBRARIES} ${_mtd_qt_libs})
endfunction()
