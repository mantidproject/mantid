# Set the version number here for MantidVersion and the package filenames
# This follows Semantic Versioning https://semver.org
set ( VERSION_MAJOR 6 )
set ( VERSION_MINOR 1 )

# UNCOMMENT the next 'set' line to 'force' the patch version number to
# a value (instead of using the count coming out of 'git describe')
# DO NOT COMMIT THIS TO MASTER UNCOMMENTED, ONLY TO A RELEASE BRANCH
# set ( VERSION_PATCH 0 )

# The tweak is mean to keep in line with the pre-release numbering.
# https://semver.org/
# examples: First release cadidate for tweak 1 is "-1-rc.1"
#           Second release cadidate for tweak 1 is "-1-rc.2"
#           Actual tweak release is "-1"
# set ( VERSION_TWEAK "-2-rc.2" )

# pep440 is incompatible with semantic versioning
# https://www.python.org/dev/peps/pep-0440/
# example: First release cadidate for tweak 1 is ".1rc.1"
if ( VERSION_TWEAK )
  string ( REPLACE "-rc."
                   "rc"
                   VERSION_TWEAK_PY
                   ${VERSION_TWEAK})
  string ( REPLACE "-"
                   "."
                   VERSION_TWEAK_PY
                   ${VERSION_TWEAK_PY})
endif()
