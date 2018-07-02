add_subdirectory( Event )
add_subdirectory(Experiment)
#add_subdirectory(Instrument)
#add_subdirectory(Runs)
add_subdirectory( RunsTable )
#add_subdirectory(Save)


set (GUI_SRC_FILES
    ${RUNS_TABLE_SRC_FILES}
    ${EVENT_SRC_FILES}
    ${EXPERIMENT_SRC_FILES}
    PARENT_SCOPE
)

set (GUI_INC_FILES
    ${RUNS_TABLE_INC_FILES}
    ${EVENT_INC_FILES}
    ${EXPERIMENT_INC_FILES}
    PARENT_SCOPE
)

set ( GUI_MOC_FILES
    ${RUNS_TABLE_MOC_FILES}
    ${EXPERIMENT_MOC_FILES}
    ${EVENT_MOC_FILES}
    PARENT_SCOPE
)

set ( GUI_UI_FILES
    ${RUNS_TABLE_UI_FILES}
    ${EXPERIMENT_UI_FILES}
    ${EVENT_UI_FILES}
    PARENT_SCOPE
)
