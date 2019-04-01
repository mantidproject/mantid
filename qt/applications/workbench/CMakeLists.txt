# Create egg link to binary output directory
add_python_package(workbench EXECUTABLE)

# Create resource files for Qt5
set(_images_qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)

# Create resources.qrc, qt.conf and qt resource.py file for developer-build
set(_paths_qrc_file_build ${CMAKE_CURRENT_LIST_DIR}/resources.qrc.build)
set(_qt_conf_file_build ${CMAKE_CURRENT_LIST_DIR}/qt.conf.build)
set(_output_res_py_build ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py)
# Write qt.conf.build
file ( WRITE ${_qt_conf_file_build} "[Paths]\nPrefix = ${THIRD_PARTY_DIR}/lib/qt5\n")
# Write resources.qrc.build
file ( WRITE ${_paths_qrc_file_build}
"<!DOCTYPE RCC><RCC version=\"1.0\">\n\
<qresource prefix=\"/qt/etc\">\n\
<file alias=\"qt.conf\">qt.conf.build</file>\n\
</qresource>\n</RCC>\n")
# Add command to generate resources.py
add_custom_command(OUTPUT ${_output_res_py_build}
                   COMMAND ${PYRCC5_CMD} -o ${_output_res_py_build} ${_images_qrc_file} ${_paths_qrc_file_build}
                   COMMENT "Generating workbench resources module"
                   DEPENDS ${_paths_qrc_file_build} ${_images_qrc_file})

# Create resources.qrc, qt.conf and qt resource.py file for package install
set(_paths_qrc_file_install ${CMAKE_CURRENT_LIST_DIR}/resources.qrc.install)
set(_qt_conf_file_install ${CMAKE_CURRENT_LIST_DIR}/qt.conf.install)
set(_output_res_py_install ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py.install)
# Write qt.conf.install
file ( WRITE ${_qt_conf_file_install} "[Paths]\nPrefix = ../lib/qt5\n")
# Write resources.qrc.install
file ( WRITE ${_paths_qrc_file_install}
"<!DOCTYPE RCC><RCC version=\"1.0\">\n\
<qresource prefix=\"/qt/etc\">\n\
<file alias=\"qt.conf\">qt.conf.install</file>\n\
</qresource>\n</RCC>\n")
# Add command to generate resources.py.install
add_custom_command(OUTPUT ${_output_res_py_install}
                   COMMAND ${PYRCC5_CMD} -o ${_output_res_py_install} ${_images_qrc_file} ${_paths_qrc_file_install}
                   COMMENT "Generating workbench resources module"
                   DEPENDS ${_paths_qrc_file_install} ${_images_qrc_file})

add_custom_target(workbench_resources ALL DEPENDS ${_output_res_py_build} ${_output_res_py_install})

# Dependency chain
add_dependencies(workbench workbench_resources mantidqt)

# ctest targets
set(TEST_FILES
    workbench/config/test/test_user.py
    workbench/test/test_import.py
    workbench/plotting/test/test_figuremanager.py
    workbench/plotting/test/test_globalfiguremanager.py
    workbench/plugins/test/test_editor.py
    workbench/plugins/test/test_exception_handler.py
    workbench/plugins/test/test_jupyterconsole.py
    workbench/plugins/test/test_workspacewidget.py
    workbench/widgets/plotselector/test/test_plotselector_model.py
    workbench/widgets/plotselector/test/test_plotselector_presenter.py
    workbench/widgets/plotselector/test/test_plotselector_view.py
    workbench/widgets/settings/test/test_settings_presenter.py
    workbench/widgets/settings/test/test_settings_view.py
    workbench/widgets/settings/general/test/test_general_settings.py
    workbench/projectrecovery/test/test_projectrecovery.py
    workbench/projectrecovery/test/test_projectrecoveryloader.py
    workbench/projectrecovery/test/test_projectrecoverysaver.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverymodel.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverypresenter.py
    workbench/projectrecovery/recoverygui/test/test_projectrecoverywidgetview.py
    workbench/projectrecovery/recoverygui/test/test_recoveryfailureview.py)

set(PYUNITTEST_RUN_SERIAL ON)
set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test (${CMAKE_CURRENT_SOURCE_DIR}
  workbench ${TEST_FILES}
)

if(CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode")
  set(PYUNITTEST_RUNNER ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/workbench-script.pyw
      -x)
else()
  set(PYUNITTEST_RUNNER ${CMAKE_BINARY_DIR}/bin/workbench -x)
endif()

# Install MantidWorkbench for OSX
if( APPLE )
  set ( MAC_BUNDLE_EXECUTABLE MantidWorkbench )
  if (OSX_VERSION VERSION_GREATER 10.7 OR OSX_VERSION VERSION_EQUAL 10.7)
    set ( MAC_BUNDLE_IDENTIFIER "org.mantidproject.MantidWorkbench" )
  else()
    set ( MAC_BUNDLE_IDENTIFIER "" )
  endif()
  set ( MAC_BUNDLE_ICON MantidWorkbench.icns )
  set ( MAC_BUNDLE_NAME MantidWorkbench )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../installers/MacInstaller/Info.plist.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
                   @ONLY )

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/make_package.rb.in
      ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb
      @ONLY )
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb DESTINATION MantidWorkbench.app/ )
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist DESTINATION MantidWorkbench.app/Contents/ )
  install (CODE "
      set (bundle \${CMAKE_INSTALL_PREFIX}/MantidWorkbench.app)
      execute_process(COMMAND chmod +x ./make_package.rb WORKING_DIRECTORY \${bundle})
      execute_process(COMMAND ./make_package.rb WORKING_DIRECTORY \${bundle} RESULT_VARIABLE install_name_tool_result)
      if(NOT install_name_tool_result EQUAL 0)
        message(FATAL_ERROR \"Package script failed!!!\n\")
      endif()
  ")

endif()
