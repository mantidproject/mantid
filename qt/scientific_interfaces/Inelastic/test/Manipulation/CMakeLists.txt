get_filename_component(SUB_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" NAME)

set(TEST_FILES
    InelasticDataManipulationElwinTabModelTest.h InelasticDataManipulationIqtTabModelTest.h
    InelasticDataManipulationMomentsTabModelTest.h InelasticDataManipulationSqwTabModelTest.h
    InelasticDataManipulationSymmetriseTabModelTest.h
)

list(TRANSFORM TEST_FILES PREPEND ${SUB_DIRECTORY}/)

set(ALL_TEST_FILES
    ${ALL_TEST_FILES} ${TEST_FILES}
    PARENT_SCOPE
)
