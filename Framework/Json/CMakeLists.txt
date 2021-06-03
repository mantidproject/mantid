set(SRC_FILES
    src/Json.cpp
)

set(INC_FILES
    inc/MantidJson/Json.h
)

set (TEST_FILES
    test/JsonTest.h
)

add_library(Json ${SRC_FILES} ${INC_FILES})
target_link_libraries(Json ${JSONCPP_LIBRARIES})
set_target_properties(Json PROPERTIES OUTPUT_NAME MantidJson)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Json
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Json
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Json PROPERTY FOLDER "MantidFramework")

# Auto-generate exports header
target_include_directories(Json PUBLIC ${CMAKE_BINARY_DIR}/Framework/Json)
generate_mantid_export_header(Json FALSE)

# Installation settings
mtd_install_targets(TARGETS
                    Json
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})