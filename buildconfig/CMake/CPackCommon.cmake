# ######################################################################################################################
# Putting all the common CPack stuff in one place
# ######################################################################################################################

# Common description stuff
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Neutron Scattering Data Reduction and Analysis")
set(CPACK_PACKAGE_VENDOR
    "ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European Spallation Source and Institut Laue - Langevin"
)
set(CPACK_PACKAGE_URL http://www.mantidproject.org/)
set(CPACK_PACKAGE_CONTACT mantid-help@mantidproject.org)
set(CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_TWEAK})
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH}${VERSION_TWEAK})

if(NOT CONDA_ENV)
  # Legacy whole bundle
  set(CPACK_MONOLITHIC_INSTALL ON)
  # Sets install for the /opt/mantid* directory
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  else()
    set(CPACK_PACKAGING_INSTALL_PREFIX "")
  endif()
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY OFF)
endif()

# RPM information - only used if generating a rpm the release number is an option set in LinuxPackageScripts.cmake
set(CPACK_RPM_PACKAGE_LICENSE GPLv3+)
set(CPACK_RPM_PACKAGE_GROUP Applications/Engineering)

# DEB information - the package does not have an original in debian archives so the debian release is 0
set(CPACK_DEBIAN_PACKAGE_RELEASE 0)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mantid Project <${CPACK_PACKAGE_CONTACT}>")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
