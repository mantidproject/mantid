macro(install_qt5_plugin_osx _qt_plugin_name _qt_plugins_var _prefix)
  get_target_property(_qt_plugin_path "${_qt_plugin_name}" LOCATION)
  if(EXISTS "${_qt_plugin_path}")
    get_filename_component(_qt_plugin_file "${_qt_plugin_path}" NAME)
    get_filename_component(_qt_plugin_type "${_qt_plugin_path}" PATH)
    get_filename_component(_qt_plugin_type "${_qt_plugin_type}" NAME)
    set(_qt_plugin_dest "${_prefix}/PlugIns/${_qt_plugin_type}")
    install(
      FILES "${_qt_plugin_path}"
      DESTINATION "${_qt_plugin_dest}"
      COMPONENT Runtime
    )
    set(${_qt_plugins_var}
        "${${_qt_plugins_var}};\$ENV{DEST_DIR}\${CMAKE_INSTALL_PREFIX}/${_qt_plugin_dest}/${_qt_plugin_file}"
    )
  else()
    message(FATAL_ERROR "QT plugin ${_qt_plugin_name} not found")
  endif()
endmacro()

install(CODE "set(WORKBENCH_APP \"${WORKBENCH_APP}\")" COMPONENT Runtime)
# Manually sort out the dependencies using cmake
install(
  CODE [[ file(GET_RUNTIME_DEPENDENCIES EXECUTABLES $<TARGET_FILE:MantidWorkbenchInstalled>
LIBRARIES $<TARGET_FILE:Qt5::QCocoaIntegrationPlugin> $<TARGET_FILE:Qt5::QMacStylePlugin> $<TARGET_FILE:Kernel> $<TARGET_FILE:API> $<TARGET_FILE:Json> $<TARGET_FILE:mantidqt_commonqt5> $<TARGET_FILE:LiveData>
RESOLVED_DEPENDENCIES_VAR _r_deps UNRESOLVED_DEPENDENCIES_VAR _u_deps
DIRECTORIES ${MY_DEPENDENCY_PATHS} )
foreach(_file ${_r_deps})
if(NOT ${_file} MATCHES "Mantid")
file(INSTALL DESTINATION
"${CMAKE_INSTALL_PREFIX}/${WORKBENCH_APP}/Contents/Frameworks" TYPE
SHARED_LIBRARY FOLLOW_SYMLINK_CHAIN FILES "${_file}" )
endif()
endforeach()
list(LENGTH _u_deps _u_length)
if("${_u_length}" GREATER 0)
message(WARNING
"Unresolved dependencies detected!")
 endif()
]]
  COMPONENT Runtime
)

install_qt5_plugin_osx("Qt5::QCocoaIntegrationPlugin" QT_PLUGINS ${CMAKE_INSTALL_PREFIX}/${WORKBENCH_APP}/Contents/)
install_qt5_plugin_osx("Qt5::QMacStylePlugin" QT_PLUGINS ${CMAKE_INSTALL_PREFIX}/${WORKBENCH_APP}/Contents/)
