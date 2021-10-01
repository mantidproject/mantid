# First generate rules for the the workbench application itself
# egg link name has to match that in setup.py name or Visual Studio generates
# a warning that the output filename of the target does not match
# the output that is produced by setuptools.
if(APPLE OR WIN32)
  if(WIN32)
    # handled separately below
    set(_exclude_on_install "resources\.pyc?")
  endif()
  add_python_package(workbench
    EXECUTABLE
    EGGLINKNAME MantidWorkbench
    EXCLUDE_FROM_INSTALL ${_exclude_on_install}
    INSTALL_LIB_DIRS "${WORKBENCH_SITE_PACKAGES}"
  )
  # Allow VS to start workbench for debugging
  set_target_properties(workbench PROPERTIES
                        VS_DEBUGGER_COMMAND $<$<BOOL:${WIN32}>:${Python_EXECUTABLE}>
                        VS_DEBUGGER_COMMAND_ARGUMENTS $<$<BOOL:${WIN32}>:${MSVC_BIN_DIR}/workbench-script.pyw>
                        VS_DEBUGGER_ENVIRONMENT $<$<BOOL:${WIN32}>:${MSVC_IDE_ENV}>)
else()
  add_python_package(workbench
    EXECUTABLE
    EGGLINKNAME MantidWorkbench
    INSTALL_LIB_DIRS "${WORKBENCH_SITE_PACKAGES}"
    INSTALL_BIN_DIR "${WORKBENCH_BIN_DIR}"
  )
endif()

if (APPLE AND Python_W_EXECUTABLE)
set(WORKBENCH_PYTHON_EXECUTABLE ${Python_W_EXECUTABLE})
else()
set(WORKBENCH_PYTHON_EXECUTABLE ${Python_EXECUTABLE})
endif()
file(TO_CMAKE_PATH ${WORKBENCH_PYTHON_EXECUTABLE} WORKBENCH_PYTHON_EXECUTABLE)

# Wrapper launchers: See mantidworkbench.cpp:main for their function
if(APPLE OR WIN32)
  # development version
  add_executable(MantidWorkbench mantidworkbench.cpp)
  target_compile_definitions(MantidWorkbench
    PRIVATE
    -DPYTHON_EXECUTABLE_PATH="${WORKBENCH_PYTHON_EXECUTABLE}")
  target_link_libraries(MantidWorkbench PRIVATE Boost::filesystem)
  set_target_properties(MantidWorkbench PROPERTIES
                        VS_DEBUGGER_ENVIRONMENT $<$<BOOL:${WIN32}>:${MSVC_IDE_ENV}>)

  # packaged version - creates a GUI application on Windows without a console
  add_executable(MantidWorkbenchInstalled WIN32 mantidworkbench.cpp)
  if(WIN32)
    set(_py_exe "pythonw.exe")
    set(_qt_plugins "..\\\\plugins\\\\qt5")
    set(PACKAGE_EXECUTABLE MantidWorkbench)
  else()
    set(_py_exe "python")
    set(_qt_plugins "../PlugIns")
    # appears in the unified menu bar and helps distiguish from other packages
    set(PACKAGE_EXECUTABLE MantidWorkbench${CPACK_PACKAGE_SUFFIX_CAMELCASE})
  endif()
  set_target_properties(MantidWorkbenchInstalled
                        PROPERTIES
                        OUTPUT_NAME ${PACKAGE_EXECUTABLE}
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  target_compile_definitions(MantidWorkbenchInstalled
    PRIVATE
    -DPYTHON_EXECUTABLE_PATH="${_py_exe}"
    -DQT_PLUGIN_PATH="${_qt_plugins}")
  target_link_libraries(MantidWorkbenchInstalled PRIVATE Boost::filesystem)
  if(WIN32)
    target_link_libraries(MantidWorkbenchInstalled PRIVATE Qt5::WinMain)
  endif()
  install(TARGETS MantidWorkbenchInstalled
          DESTINATION ${WORKBENCH_BIN_DIR})
endif()

# process resources files
set(_images_qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)
if (WIN32)
  function (CREATE_QRC_FILE_TEMPLATE BUILD_TYPE)
    # Set path and file content variables
    set(_paths_qrc_file ${CMAKE_CURRENT_BINARY_DIR}/resources.qrc.${BUILD_TYPE})
    set(_qt_conf_file ${CMAKE_CURRENT_BINARY_DIR}/qt.conf.${BUILD_TYPE})
    set(_qrc_file_contents "<!DOCTYPE RCC><RCC version=\"1.0\">\n<qresource prefix=\"/qt/etc\">\n<file alias=\"qt.conf\">qt.conf.${BUILD_TYPE}</file>\n</qresource>\n</RCC>\n")
    if (CONDA_BUILD)
      file(TO_CMAKE_PATH $ENV{CONDA_PREFIX}/Library _qt5_dir)
    else()
      set(_qt5_dir ${THIRD_PARTY_DIR}/lib/qt5)
    endif()
    # Write qt.conf.BUILD_TYPE file and get resources file name
    if (${BUILD_TYPE} STREQUAL "build")
      file (WRITE ${_qt_conf_file} "[Paths]\nPrefix = ${_qt5_dir}\n\n[Platforms]\nWindowsArguments = dpiawareness=1\n")
      set(_output_res_py ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py)
    elseif(${BUILD_TYPE} STREQUAL "install")
      file (WRITE ${_qt_conf_file} "[Paths]\nPrefix = ../lib/qt5\n\n[Platforms]\nWindowsArguments = dpiawareness=1\n")
      file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/qt.conf.webengine "[Paths]\nPrefix = ../\n")
      set(_output_res_py ${CMAKE_CURRENT_BINARY_DIR}/resources.py.install)
    endif()

    # Write resources.qrc.BUILD_TYPE
    file (WRITE ${_paths_qrc_file} ${_qrc_file_contents})

    # Add command to generate resources.py file
    add_custom_command(OUTPUT ${_output_res_py}
                      COMMAND ${PYRCC5_CMD} -o ${_output_res_py} ${_images_qrc_file} ${_paths_qrc_file}
                      COMMENT "Generating workbench resources module for ${BUILD_TYPE}: ${_output_res_py}"
                      DEPENDS ${_images_qrc_file} ${_paths_qrc_file})
  endfunction()

  # Run function for each build type
  foreach(BUILD_TYPE "build" "install")
    CREATE_QRC_FILE_TEMPLATE(${BUILD_TYPE})
  endforeach()
  add_custom_target(workbench_resources ALL
                    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/resources.py.install ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py)
  # Install rule for package's resources.py
  install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/resources.py.install
            DESTINATION ${WORKBENCH_SITE_PACKAGES}/workbench/app RENAME resources.py)
else()
  if (CONDA_BUILD)
  set (_output_res_py ${WORKBENCH_SITE_PACKAGES}/workbench/app/resources.py)
  else()
  set (_output_res_py ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py)
  endif()
  add_custom_command(OUTPUT ${_output_res_py}
                     COMMAND ${PYRCC5_CMD} -o ${_output_res_py} ${_images_qrc_file}
                     COMMENT "Generating workbench resources module"
                     DEPENDS ${_images_qrc_file})
  add_custom_target(workbench_resources ALL DEPENDS ${_output_res_py})
endif()

# Dependency chain
add_dependencies(workbench workbench_resources)
# if we are building mantidqt we can add a dependency for workbench
if (MANTID_QT_LIB STREQUAL "BUILD")
add_dependencies(workbench mantidqt)
endif()

# macOS package fixes
if(APPLE AND NOT CONDA_BUILD)
  # create Info.plist
  set ( MAC_BUNDLE_EXECUTABLE ${PACKAGE_EXECUTABLE})
  set ( MAC_BUNDLE_IDENTIFIER "org.mantidproject.${PACKAGE_EXECUTABLE}" )
  set ( MAC_BUNDLE_ICON mantid_workbench${CPACK_PACKAGE_SUFFIX}.icns )
  set ( MAC_BUNDLE_NAME ${PACKAGE_EXECUTABLE} )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/../../../installers/MacInstaller/Info.plist.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
                   @ONLY )
  # Determine Qt install prefix
  set(_qt_install_prefix ${Qt5Core_DIR})
  foreach(_i RANGE 2)
    get_filename_component(_qt_install_prefix ${_qt_install_prefix} DIRECTORY)
  endforeach()
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Info.plist DESTINATION ${WORKBENCH_BUNDLE} )
  install (CODE "
      execute_process(COMMAND ${CMAKE_SOURCE_DIR}/installers/MacInstaller/make_package.rb
                              \${CMAKE_INSTALL_PREFIX}/${WORKBENCH_APP} ${Python_EXECUTABLE} ${_qt_install_prefix}
                      RESULT_VARIABLE install_name_tool_result OUTPUT_VARIABLE _out ERROR_VARIABLE _out COMMAND_ECHO STDOUT)
      if(NOT install_name_tool_result EQUAL 0)
        message(\"\${_out}\")
        message(FATAL_ERROR \"Package script failed!!!\n\")
      endif()
  ")
endif()

# Testing
set(TEST_FILES
    workbench/config/test/test_user.py
    workbench/test/test_mainwindow.py
    workbench/test/test_import.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratoraxes.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorcolorfills.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorfigure.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorlegend.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorlines.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorutils.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgeneratorfitting.py
    workbench/plotting/plotscriptgenerator/test/test_plotscriptgenerator.py
    workbench/plotting/test/test_figureerrorsmanager.py
    workbench/plotting/test/test_figureinteraction.py
    workbench/plotting/test/test_figuremanager.py
    workbench/plotting/test/test_figurewindow.py
    workbench/plotting/test/test_globalfiguremanager.py
    workbench/plotting/test/test_propertiesdialog.py
    workbench/plotting/test/test_plothelppages.py
    workbench/plotting/test/test_toolbar.py
    workbench/plotting/test/test_utility.py
    workbench/plugins/test/test_editor.py
    workbench/plugins/test/test_exception_handler.py
    workbench/plugins/test/test_jupyterconsole.py
    workbench/plugins/test/test_workspacewidget.py
    workbench/utils/test/test_workspacehistorygeneration.py
    workbench/widgets/about/test/test_about_presenter.py
    workbench/widgets/plotselector/test/test_plotselector_model.py
    workbench/widgets/plotselector/test/test_plotselector_presenter.py
    workbench/widgets/plotselector/test/test_plotselector_view.py
    workbench/widgets/settings/test/test_settings_model.py
    workbench/widgets/settings/test/test_settings_presenter.py
    workbench/widgets/settings/test/test_settings_view.py
    workbench/widgets/settings/general/test/test_general_settings.py
    workbench/widgets/settings/plots/test/test_plot_settings.py
    workbench/widgets/settings/categories/test/test_categories_settings.py
    workbench/widgets/settings/fitting/test/test_fitting_settings.py
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
