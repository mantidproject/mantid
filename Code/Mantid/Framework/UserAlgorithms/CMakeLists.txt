set ( SRC_FILES HelloWorldAlgorithm.cpp
                LorentzianTest.cpp
                ModifyData.cpp
                PropertyAlgorithm.cpp
                WorkspaceAlgorithm.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES HelloWorldAlgorithm.h
                LorentzianTest.h
                ModifyData.h
                PropertyAlgorithm.h
                WorkspaceAlgorithm.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(UserAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 1000)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( UserAlgorithms EXCLUDE_FROM_ALL ${SRC_FILES} ${INC_FILES})

# Add to the 'FrameworkTests' group in VS
set_property ( TARGET UserAlgorithms PROPERTY FOLDER "UnitTests" )

target_link_libraries ( UserAlgorithms ${MANTIDLIBS} )
