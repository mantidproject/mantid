# Try to find the patchelf executable.
# http://nixos.org/patchelf.html
# Source in SVN under TestingTools/patchelf-0.5
#
find_program ( PATCHELF_PROG NAMES patchelf )

# handle the QUIETLY and REQUIRED arguments and set QWT_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Patchelf DEFAULT_MSG PATCHELF_PROG PATCHELF_FOUND )

mark_as_advanced ( PATCHELF_PROG PATCHELF_FOUND )
