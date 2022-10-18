# Set the version number here for MantidVersion and the package filenames. This roughly follows Semantic Versioning
# https://semver.org, but will adhere to PEP440 https://peps.python.org/pep-0440/
#
# The version number will be of the form
#
# major.minor.patch(tweak(rcN))
#
# where tweak version and rcN number are optional. This assumes the rc value will not exist without a tweak version.
# Examples: `6.5.0`, `6.4.20220915.1529`, `6.4.0.1`, and `6.4.0.1rc2`.

if(CONDA_BUILD)
  # If inside of conda-build assume the version number is set by the version number in the recipe and brough in by the
  # PKG_VERSION environment variable that conda sets
  # https://conda.io/projects/conda-build/en/latest/user-guide/environment-variables.html. The conda recipe version
  # number is set either manually in https://github.com/mantidproject/conda-recipes.git and Automatically set by the
  # nightly pipeline that handles conda-build.
  set(_conda_package_version $ENV{PKG_VERSION})
  # convert the `.` into `;` so cmake treats it as a list
  string(REPLACE "." ";" _conda_package_version ${_conda_package_version})
  set(_conda_package_list ${_conda_package_version})

  # the length of the list declares whether there is a tweak version
  list(LENGTH _conda_package_version _number_version_level)

  # unpack the list into major.minor.patch(tweak)
  list(GET _conda_package_version 0 VERSION_MAJOR)
  list(GET _conda_package_version 1 VERSION_MINOR)
  list(GET _conda_package_version 2 VERSION_PATCH)
  if(_number_version_level GREATER 3)
    list(GET _conda_package_version 3 VERSION_TWEAK)
    # The version number in template files (e.g. MantidVersion.cpp.in) assumes that the `.` is included in the tweak
    # number
    set(VERSION_TWEAK ".${VERSION_TWEAK}")
  endif()

else()
  set(VERSION_MAJOR 6)
  set(VERSION_MINOR 5)

  # UNCOMMENT the next 'set' line to 'force' the patch version number to a value (instead of using the count coming out
  # of 'git describe') DO NOT COMMIT THIS TO MASTER UNCOMMENTED, ONLY TO A RELEASE BRANCH

  # set(VERSION_PATCH 0)

  # The tweak is mean to keep in line with the pre-release numbering. https://semver.org/ examples: First release
  # cadidate for tweak 1 is ".1rc1" Second release cadidate for tweak 1 is ".1rc2" Actual tweak release is ".1"

  # set(VERSION_TWEAK ".2rc1")

endif()
message(STATUS "Version: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_TWEAK}")
