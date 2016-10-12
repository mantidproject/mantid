###############################################################################
# Putting all the common CPack stuff in one place
###############################################################################

# Common description stuff
set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Neutron Scattering Data Analysis" )
set ( CPACK_PACKAGE_VENDOR "ISIS Rutherford Appleton Laboratory and NScD Oak Ridge National Laboratory" )
set ( CPACK_PACKAGE_URL http://www.mantidproject.org/ )
set ( CPACK_PACKAGE_CONTACT mantid-help@mantidproject.org )
set ( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH} )
set ( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
set ( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
set ( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH} )

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows") # To avoid breaking Windows vates packaging
  set ( CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
endif()

# RPM information - only used if generating a rpm
set ( CPACK_RPM_PACKAGE_LICENSE GPLv3+ )
set ( CPACK_RPM_PACKAGE_RELEASE 1 )
set ( CPACK_RPM_PACKAGE_GROUP Applications/Engineering )

# DEB informatin - the package does not have an original
# in debian archives so the debian release is 0
set ( CPACK_DEBIAN_PACKAGE_RELEASE 0 )
