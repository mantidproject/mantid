add_subdirectory( QtWidgets )
include_directories( QtWidgets/inc )

set_mantid_subprojects( Vates/VatesAPI )
add_subdirectory( ViewWidgets )
include_directories( ViewWidgets/inc )

add_subdirectory( StandAloneExec )
