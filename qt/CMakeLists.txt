if(ENABLE_MANTIDPLOT)
  find_package(QScintillaQt4 REQUIRED)
endif()

# Utilities for defining targets
include(QtTargetFunctions)

# Resource compiler for PyQt5
if(ENABLE_WORKBENCH AND NOT PYRCC5_CMD)
  # Newer versions of PyQt5 have a pyrcc_main python module, whereas older
  # versions have a pyrcc5 executable. We prefer calling the python module.
  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import PyQt5.pyrcc_main"
                  RESULT_VARIABLE _status)
  if(_status EQUAL 0)
    set(PYRCC5_CMD
        ${PYTHON_EXECUTABLE}
        -m
        PyQt5.pyrcc_main
        CACHE string "Command line to use to run the PyQt5 resource compiler")
  else()
    find_file(PYRCC5_CMD "pyrcc5")
    if(NOT PYRCC5_CMD)
      message(
        FATAL_ERROR
          "Unable to find PyQt5 resource compiler. Tried PyQt5.pyrcc_main & pyrcc5"
        )
    endif()
  endif()
endif()

# Qt-based targets
add_subdirectory(icons)
add_subdirectory(widgets)
add_subdirectory(python)
add_subdirectory(scientific_interfaces)
if(MAKE_VATES)
  add_subdirectory(paraview_ext)
endif(MAKE_VATES)

add_subdirectory(applications)
