# Utilities for defining targets
include(QtTargetFunctions)

# Resource compiler for PyQt5. PyQt6 does not ship a Python resource compiler (pyrcc was removed in Qt6), so this is
# only wired up for the Qt5 build; the Qt6 build compiles a binary .rcc with Qt's own rcc instead (see below).
if((ENABLE_WORKBENCH OR BUILD_MANTIDQT)
   AND NOT PYRCC5_CMD
   AND MANTID_QT_VERSION EQUAL 5
)
  # Newer versions of PyQt5 have a pyrcc_main python module, whereas older versions have a pyrcc5 executable. We prefer
  # calling the python module.
  execute_process(COMMAND ${Python_EXECUTABLE} -c "import PyQt5.pyrcc_main" RESULT_VARIABLE _status)
  if(_status EQUAL 0)
    set(PYRCC5_CMD
        ${Python_EXECUTABLE} -m PyQt5.pyrcc_main
        CACHE STRING "Command line to use to run the PyQt5 resource compiler"
    )
  else()
    find_file(PYRCC5_CMD "pyrcc5")
    if(NOT PYRCC5_CMD)
      message(FATAL_ERROR "Unable to find PyQt5 resource compiler. Tried PyQt5.pyrcc_main & pyrcc5")
    endif()
  endif()
endif()

# Qt6 has no Python resource compiler, so we cannot generate a resources.py module. Instead use Qt's own rcc to compile
# a binary .rcc that is registered at runtime via QResource.registerResource (see qt/applications/workbench). rcc is a
# host tool provided by the Qt6::Core package found above.
if((ENABLE_WORKBENCH OR BUILD_MANTIDQT) AND MANTID_QT_VERSION EQUAL 6)
  if(TARGET Qt6::rcc)
    set(QT_RCC_CMD $<TARGET_FILE:Qt6::rcc>)
  else()
    find_program(
      QT_RCC_CMD
      NAMES rcc
      HINTS ${Qt6_DIR}/../../qt6 ${Qt6_DIR}/../../../libexec ${Qt6_DIR}/../../../bin
    )
    if(NOT QT_RCC_CMD)
      message(WARNING "Qt6 rcc not found: workbench resources will not be compiled")
    endif()
  endif()
endif()

# Include python helper functions
include(PythonPackageTargetFunctions)

# Qt-based targets
if(BUILD_MANTIDQT)
  add_subdirectory(icons)
  add_subdirectory(widgets)
  add_subdirectory(python)
  add_subdirectory(scientific_interfaces)
endif()
if(ENABLE_WORKBENCH)
  add_subdirectory(applications)
endif()
