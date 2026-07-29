# Utilities for defining targets
include(QtTargetFunctions)

# Qt6 has no Python resource compiler (pyrcc was removed in Qt6), so we cannot generate a resources.py module. Instead
# use Qt's own rcc to compile a binary .rcc that is registered at runtime via QResource.registerResource (see
# qt/applications/workbench). rcc is a host tool provided by the Qt6::Core package found above.
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
