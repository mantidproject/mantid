###############################################################################
# Putting the Linux specific CPack stuff here
###############################################################################

# Create a package file name for the Linux distributions
string ( TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME )

# define the source generators
set ( CPACK_SOURCE_GENERATOR "TGZ;TXZ" )
set ( CPACK_SOURCE_IGNORE_FILES "/\\\\.git*")
if (CMAKE_BINARY_DIR MATCHES "^${CMAKE_SOURCE_DIR}/.+")
  # In-source build add the binary directory to files to ignore for the tarball
  string (REGEX REPLACE "^${CMAKE_SOURCE_DIR}/([^\\/]+)(.*)\$" "/\\1/" _ignore_dir "${CMAKE_BINARY_DIR}")
  set (CPACK_SOURCE_IGNORE_FILES "${CPACK_SOURCE_IGNORE_FILES};${_ignore_dir}")
endif ()

include (DetermineLinuxDistro)

# define which binary generators to use
if ( "${UNIX_DIST}" MATCHES "Ubuntu" )
  find_program (DPKG_CMD dpkg)
  if ( DPKG_CMD )
    set ( CPACK_GENERATOR "DEB" )
    set ( CPACK_DEBIAN_COMPRESSION_TYPE "xz" )
    execute_process( COMMAND ${DPKG_CMD} --print-architecture
      OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE )
    # following Ubuntu convention <foo>_<VersionNumber>-<DebianRevisionNumber>ubuntu1~<UbuntuCodeName>1_<DebianArchitecture>.deb
    set ( UBUNTU_PACKAGE_RELEASE "ubuntu1~${UNIX_CODENAME}1" )
    set ( CPACK_PACKAGE_FILE_NAME
          "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}${UBUNTU_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  endif ( DPKG_CMD )
endif ( "${UNIX_DIST}" MATCHES "Ubuntu" )

#RedHatEnterpriseClient RedHatEnterpriseWorkstation
if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" OR "${UNIX_DIST}" MATCHES "Fedora" OR "${UNIX_DIST}" MATCHES "SUSE LINUX" )
  find_program ( RPMBUILD_CMD rpmbuild )
  if ( RPMBUILD_CMD )
    set ( CPACK_GENERATOR "RPM" )
    set ( CPACK_RPM_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}" )
    set ( CPACK_RPM_PACKAGE_URL "http://www.mantidproject.org" )
    set ( CPACK_RPM_COMPRESSION_TYPE "xz" )

    # determine the distribution number
    if(NOT CPACK_RPM_DIST)
      execute_process(COMMAND ${RPMBUILD_CMD} -E %{?dist}
                      OUTPUT_VARIABLE CPACK_RPM_DIST
                      ERROR_QUIET
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    # release number defaults to 1
    if(NOT CPACK_RPM_PACKAGE_RELEASE_NUMBER)
      set(CPACK_RPM_PACKAGE_RELEASE_NUMBER "1")
    endif()

    # reset the release name
    set( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE_NUMBER}${CPACK_RPM_DIST}" )

    # If CPACK_SET_DESTDIR is ON then the Prefix doesn't get put in the spec file
    if( CPACK_SET_DESTDIR )
      message ( STATUS "Adding \"Prefix:\" line to spec file manually when CPACK_SET_DESTDIR is set")
      set( CPACK_RPM_SPEC_MORE_DEFINE "Prefix: ${CPACK_PACKAGING_INSTALL_PREFIX}" )
    endif()

    # according to rpm.org: name-version-release.architecture.rpm
    set ( CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE}" )
  endif ( RPMBUILD_CMD)
endif ()
