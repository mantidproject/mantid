set(VSI_GLOBAL_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/test/global/scripts)

add_subdirectory(QtWidgets)
include_directories(QtWidgets/inc)

set_mantid_subprojects(qt/paraview_ext/VatesAPI)
add_subdirectory(ViewWidgets)
include_directories(ViewWidgets/inc)
