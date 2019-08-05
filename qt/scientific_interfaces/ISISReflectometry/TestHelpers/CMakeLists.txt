set(TESTHELPERS_SRC_FILES ModelCreationHelper.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(TESTHELPERS_INC_FILES ModelCreationHelper.h)

prepend(
  TESTHELPERS_SRC_FILES
  TestHelpers
  ${TESTHELPERS_SRC_FILES}
)
prepend(
  TESTHELPERS_INC_FILES
  TestHelpers
  ${TESTHELPERS_INC_FILES}
)

set(TESTHELPERS_SRC_FILES ${TESTHELPERS_SRC_FILES} PARENT_SCOPE)
set(TESTHELPERS_INC_FILES ${TESTHELPERS_INC_FILES} PARENT_SCOPE)
