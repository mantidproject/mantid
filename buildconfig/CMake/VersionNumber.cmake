# Set the version number here for MantidVersion and the package filenames This follows Semantic Versioning
# https://semver.org
if(CONDA_BUILD)
  set(CONDA_PACKAGE_VERSION $ENV{PKG_VERSION})
  string(REPLACE "." ";" CONDA_PACKAGE_LIST ${CONDA_PACKAGE_VERSION})

  list(GET CONDA_PACKAGE_VERSION 0 VERSION_MAJOR)
  list(GET CONDA_PACKAGE_VERSION 1 VERSION_MINOR)
  list(SUBLIST ${CONDA_PACKAGE_LIST} 2 -1 VERSION_PATCH_LIST)
  set(VERSION_PATCH)
  string(REPLACE ";" "" VERSION_PATCH "${VERSION_PATCH_LIST}")
  message(STATUS "Version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
else()
  set(VERSION_MAJOR 6)
  set(VERSION_MINOR 2)
endif()

# UNCOMMENT the next 'set' line to 'force' the patch version number to a value (instead of using the count coming out of
# 'git describe') DO NOT COMMIT THIS TO MASTER UNCOMMENTED, ONLY TO A RELEASE BRANCH

# set ( VERSION_PATCH 0 )

# The tweak is mean to keep in line with the pre-release numbering. https://semver.org/ examples: First release cadidate
# for tweak 1 is "-1-rc.1" Second release cadidate for tweak 1 is "-1-rc.2" Actual tweak release is "-1"

# set(VERSION_TWEAK "-4-rc.2")

# pep440 is incompatible with semantic versioning https://www.python.org/dev/peps/pep-0440/ example: First release
# cadidate for tweak 1 is ".1rc.1"
if(VERSION_TWEAK)
  string(REPLACE "-rc." "rc" VERSION_TWEAK_PY ${VERSION_TWEAK})
  string(REPLACE "-" "." VERSION_TWEAK_PY ${VERSION_TWEAK_PY})
endif()
