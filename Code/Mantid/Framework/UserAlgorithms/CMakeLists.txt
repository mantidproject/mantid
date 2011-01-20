set ( SRC_FILES HelloWorldAlgorithm.cpp
                LorentzianTest.cpp
                ModifyData.cpp
                PropertyAlgorithm.cpp
                WorkspaceAlgorithm.cpp
)

set ( INC_FILES HelloWorldAlgorithm.h
                LorentzianTest.h
                ModifyData.h
                PropertyAlgorithm.h
                WorkspaceAlgorithm.h
)

# Add the target for this directory
add_library ( UserAlgorithms EXCLUDE_FROM_ALL ${SRC_FILES} ${INC_FILES})

# Add to the 'FrameworkTests' group in VS
set_property ( TARGET UserAlgorithms PROPERTY FOLDER "FrameworkTests" )

target_link_libraries ( UserAlgorithms ${MANTIDLIBS} )
