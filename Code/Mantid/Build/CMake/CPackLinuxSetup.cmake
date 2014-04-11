###############################################################################
# Putting the Linux specific CPack stuff here
###############################################################################

# Create a package file name for the Linux distributions
string ( TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME )

# define the source generators
set ( CPACK_SOURCE_GENERATOR TGZ )

include (DetermineLinuxDistro)

# define which binary generators to use
if ( ${UNIX_DIST} MATCHES "Ubuntu" )
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
endif ( ${UNIX_DIST} MATCHES "Ubuntu" )

#RedHatEnterpriseClient RedHatEnterpriseWorkstation
if ( ${UNIX_DIST} MATCHES "RedHatEnterprise" OR ${UNIX_DIST} MATCHES "Fedora" OR ${UNIX_DIST} MATCHES "SUSE LINUX" )
  find_program ( RPMBUILD_CMD rpmbuild )
  if ( RPMBUILD_CMD )
    set ( CPACK_GENERATOR "RPM" )
    set ( CPACK_RPM_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}" )
    # reset the release name to include the RHEL version if known
    if ( ${UNIX_CODENAME} MATCHES "Tikanga" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.el5" )
    elseif ( ${UNIX_CODENAME} MATCHES "Santiago" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.el6" )
    elseif ( ${UNIX_CODENAME} MATCHES "Laughlin" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc14" )
    elseif ( ${UNIX_CODENAME} MATCHES "Lovelock" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc15" )
    elseif ( ${UNIX_CODENAME} MATCHES "Verne" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc16" )
    elseif ( ${UNIX_CODENAME} MATCHES "BeefyMiracle" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc17" )
    elseif ( ${UNIX_CODENAME} MATCHES "SphericalCow" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc18" )
    elseif ( ${UNIX_CODENAME} MATCHES "Heisenbug" )
      set ( CPACK_RPM_PACKAGE_RELEASE "${CPACK_RPM_PACKAGE_RELEASE}.fc20" )
    endif ( ${UNIX_CODENAME} MATCHES "Tikanga" )
    
    # If CPACK_SET_DESTDIR is ON then the Prefix doesn't get put in the spec file
    if( CPACK_SET_DESTDIR )
      message ( "Adding \"Prefix:\" line to spec file manually when CPACK_SET_DESTDIR is set")
      set( CPACK_RPM_SPEC_MORE_DEFINE "Prefix: ${CPACK_PACKAGING_INSTALL_PREFIX}" )
    endif()

    # according to rpm.org: name-version-release.architecture.rpm
    set ( CPACK_PACKAGE_FILE_NAME
      "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}.${CPACK_RPM_PACKAGE_ARCHITECTURE}" )
  endif ( RPMBUILD_CMD)
endif ( ${UNIX_DIST} MATCHES "RedHatEnterprise" OR ${UNIX_DIST} MATCHES "Fedora" OR ${UNIX_DIST} MATCHES "SUSE LINUX" )

