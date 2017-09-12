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
  #mtd_add_qt_target (EXECUTABLE QT_VERSION 5 ${ARGN})
endfunction()

# Target agnostic function to add either an executable or library linked to Qt
# option: LIBRARY If included define a library target
# option: EXECUTABLE If included define an executable target
# option: NO_SUFFIX If included, no suffix is added to the target name
# option: EXCLUDE_FROM_ALL If included, the target is excluded from target ALL
# keyword: TARGET_NAME The name of the target. The target will have -Qt{QT_VERSION} appended to it.
# keyword: QT_VERSION The major version of Qt to build against
# keyword: SRC .cpp files to include in the target build
# keyword: MOC Header files that are to be parsed by moc
# keyword: UI Qt designer ui files that are to be parsed by the UI compiler
# keyword: NOMOC Additional headers that are not to be passed to moc
# keyword: RES Any resource .qrc files
# keyword: DEFS Compiler definitions to set to the target
# keyword: QT_PLUGIN If included, the target is a Qt plugin. The value is the name of the plugin library
# keyword: INCLUDE_DIRS A list of include directories to add to the target
# keyword: PRECOMPILED A name of the precompiled header
# keyword: LINK_LIBRARIES A list of additional libraries to link to the
#          target that are not dependent on Qt
# keyword: MTD_QT_LINK_LIBRARIES A list of additional libraries to link to the
#          target. It is assumed each was produced with this function and
#          will have the -Qt{QT_VERSION} suffix appended.
# keyword: INSTALL_DIR A destination directory for the install command.
# keyword: OSX_INSTALL_RPATH Install path for osx version > 10.8
function (mtd_add_qt_target)
  set (options LIBRARY EXECUTABLE NO_SUFFIX EXCLUDE_FROM_ALL)
  set (oneValueArgs TARGET_NAME QT_VERSION QT_PLUGIN INSTALL_DIR OSX_INSTALL_RPATH PRECOMPILED)
  set (multiValueArgs SRC UI MOC NOMOC RES DEFS INCLUDE_DIRS LINK_LIBRARIES MTD_QT_LINK_LIBRARIES)
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
    qt4_add_resources (RES_FILES ${PARSED_RES})
    set (_qt_link_libraries Qt4::QtGui)
  elseif (PARSED_QT_VERSION EQUAL 5)
    qt5_wrap_ui (UI_HEADERS ${PARSED_UI})
    qt5_wrap_cpp (MOC_GENERATED ${PARSED_MOC})
    qt5_add_resources (RES_FILES ${PARSED_RES})
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
  if (${PARSED_EXCLUDE_FROM_ALL})
    set(_target_exclude_from_all "EXCLUDE_FROM_ALL")
  else()
    set(_target_exclude_from_all)
  endif()

  set(ALL_SRC ${PARSED_SRC} ${MOC_GENERATED})
  # Use a precompiled header where they are supported
  if (${PARSED_PRECOMPILED})
    enable_precompiled_headers( ${PARSED_PRECOMPILED}  ALL_SRC )
  endif()

  if (${PARSED_LIBRARY})
    add_library (${_target} ${_target_exclude_from_all} ${ALL_SRC} ${UI_HEADERS} ${PARSED_NOMOC} ${RES_FILES})
  elseif (${PARSED_EXECUTABLE})
    add_executable (${_target} ${_target_exclude_from_all} ${ALL_SRC} ${UI_HEADERS} ${PARSED_NOMOC} ${RES_FILES})
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
  if(PARSED_DEFS)
    set_target_properties ( ${_target} PROPERTIES COMPILE_DEFINITIONS ${PARSED_DEFS} )
  endif()
  if (OSX_VERSION VERSION_GREATER 10.8)
    if (NOT ${PARSED_OSX_INSTALL_RPATH})
        set(PARSED_OSX_INSTALL_RPATH "@loader_path/../MacOS")
    endif()
    set_target_properties ( ${_target} PROPERTIES INSTALL_RPATH ${PARSED_OSX_INSTALL_RPATH})
  endif ()

  if (PARSED_QT_PLUGIN)
    # Define a compiler variable to set the name of the library within the code. This
    # is required by the Qt plugin mechanism
    #set( LIB_NAME MantidWidgetPlugins )
    add_definitions( -DLIBRARY_NAME=${PARSED_QT_PLUGIN} )

    # Change the destination of the target as Qt expects this in a directory called "designer"
    # if you set QT_PLUGIN_PATH environment variable this might put it there???
    SET_TARGET_OUTPUT_DIRECTORY( ${_target} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/designer )

    # Set the name of the generated library
    set_target_properties ( ${_target} PROPERTIES OUTPUT_NAME ${PARSED_QT_PLUGIN} )
  endif()
  
  if (NOT ${PARSED_EXCLUDE_FROM_ALL})
    if (NOT ${PARSED_INSTALL_DIR})
        set(INSTALL_DESTINATION_DIR ${PARSED_INSTALL_DIR})
    else()
        set(INSTALL_DESTINATION_DIR ${LIB_DIR})
    endif()
    install ( TARGETS ${_target} ${SYSTEM_PACKAGE_TARGET} DESTINATION ${INSTALL_DESTINATION_DIR} )
  endif()
endfunction()
