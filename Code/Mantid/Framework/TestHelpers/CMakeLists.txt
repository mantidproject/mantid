
#======== TARGET FOR A WIKI PAGE MAKER =================
add_executable(WikiMaker EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/src/WikiMaker.cpp )
target_link_libraries ( WikiMaker ${MANTIDLIBS} Algorithms DataHandling MDEvents)

set ( TEST_FILES
)
