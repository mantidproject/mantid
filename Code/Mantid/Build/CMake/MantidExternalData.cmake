################################################################################
# Setup the ExternalData variables for the project
################################################################################
include(ExternalData)

if(NOT MANTID_DATA_STORE)
  # Select a default in the home directory
  set(MANTID_DATA_STORE_DEFAULT "$ENV{HOME}/MantidExternalData")
endif()

# Provide users with an option to select a local object store,
# starting with the above-selected default.
set(MANTID_DATA_STORE "${MANTID_DATA_STORE_DEFAULT}" CACHE PATH
  "Local directory holding ExternalData objects in the layout %(algo)/%(hash).")
mark_as_advanced(MANTID_DATA_STORE)

if(NOT MANTID_DATA_STORE)
  message(FATAL_ERROR "MANTID_DATA_STORE not set. It is required for external data")
endif()

# Tell ExternalData module about selected object stores.
list(APPEND ExternalData_OBJECT_STORES
  # Store selected by Mantid-specific configuration above.
  ${MANTID_DATA_STORE}
)

# Default binary root to build directory
set(ExternalData_BINARY_ROOT ${CMAKE_BINARY_DIR}/ExternalData CACHE STRING
  "A directory holding the links (copies on windows) to the real content files.")

set(ExternalData_URL_TEMPLATES "" CACHE STRING
  "Additional URL templates for the ExternalData CMake script to look for testing data. E.g.
file:///var/bigharddrive/%(algo)/%(hash)")
mark_as_advanced(ExternalData_URL_TEMPLATES)
list(APPEND ExternalData_URL_TEMPLATES
  "http://198.74.56.37/ftp/external-data/%(algo)/%(hash)"
)

