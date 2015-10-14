###############################################################################
# Putting the Linux specific CPack stuff here
###############################################################################

# Create a package file name for the Linux distributions
string ( TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME )

# define the source generators
set ( CPACK_SOURCE_GENERATOR TGZ )

include (DetermineLinuxDistro)

# define which binary generators to use
if ( "${UNIX_DIST}" MATCHES "Ubuntu" )
  find_program (DPKG_CMD dpkg)
  if ( DPKG_CMD )
    set ( CPACK_GENERATOR "DEB" )
    execute_process( COMMAND ${DPKG_CMD} --print-architecture
      OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE )
    # according to debian <foo>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb 
    set( CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")   
  endif ( DPKG_CMD ) 
endif ( "${UNIX_DIST}" MATCHES "Ubuntu" )

#RedHatEnterpriseClient RedHatEnterpriseWorkstation
if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" OR "${UNIX_DIST}" MATCHES "Fedora" OR "${UNIX_DIST}" MATCHES "SUSE LINUX" )
  find_program ( RPMBUILD_CMD rpmbuild )
  if ( RPMBUILD_CMD )
    set ( CPACK_GENERATOR "RPM" )
    set ( CPACK_RPM_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}" )
    set ( CPACK_RPM_PACKAGE_URL "http://www.mantidproject.org" )

    # reset the release name to include the RHEL version if known
    if ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" )
      string ( REGEX REPLACE "^([0-9])\\.[0-9]+$" "\\1" TEMP ${UNIX_RELEASE} )
      set ( CPACK_RPM_PACKAGE_RELEASE "1.el${TEMP}" )
    elseif ( "${UNIX_DIST}" MATCHES "Fedora" )
      set ( CPACK_RPM_PACKAGE_RELEASE "1.fc${UNIX_RELEASE}" )
    endif ( "${UNIX_DIST}" MATCHES "RedHatEnterprise" )
    
    # If CPACK_SET_DESTDIR is ON then the Prefix doesn't get put in the spec file
    if( CPACK_SET_DESTDIR )
      message ( "Adding \"Prefix:\" line to spec file manually when CPACK_SET_DESTDIR is set")
      set( CPACK_RPM_SPEC_MORE_DEFINE "Prefix: ${CPACK_PACKAGING_INSTALL_PREFIX}" )
    endif()

    # according to rpm.org: name-version-release.architecture.rpm
    set ( CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE}" )
  endif ( RPMBUILD_CMD)
endif ()

