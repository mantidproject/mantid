#.rst:
# FindGitLFS
# ----------
#
#
#
# The module defines the following variables:
#
# ::
#
#    GITLFS_EXECUTABLE - path to git-lfs command line client
#    GITLFS_FOUND - true if the command line client was found
#
# Example usage:
#
# ::
#
#    find_package(GitLFS)
#    if(GITLFS_FOUND)
#      message("git found: ${GITLFS_EXECUTABLE}")
#    endif()

set(gitlfs_names git-lfs )

find_program(GITLFS_EXECUTABLE
  NAMES ${gitlfs_names}
  PATH_SUFFIXES Git/bin Git/mingw64/bin
  DOC "git-lfs command line client"
  )
mark_as_advanced(GITLFS_EXECUTABLE)

# Handle the QUIETLY and REQUIRED arguments and set GIT_FOUND to TRUE if
# all listed variables are TRUE

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GitLFS
                                  REQUIRED_VARS GITLFS_EXECUTABLE)
