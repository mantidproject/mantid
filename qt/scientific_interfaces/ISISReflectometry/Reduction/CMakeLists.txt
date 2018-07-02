set ( REDUCTION_SRC_FILES
    Group.cpp
    RangeInQ.cpp
    ReductionWorkspaces.cpp
    Row.cpp
    SlicedReductionWorkspaces.cpp
    ReductionJobs.cpp
    ValidateRow.cpp
    Slicing.cpp

    Experiment.cpp
    PolarizationCorrections.cpp
    RangeInLambda.cpp
    RowTemplate.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( REDUCTION_INC_FILES
    Group.h
    RangeInQ.h
    ReductionJobs.h
    ReductionWorkspaces.h
    Row.h
    SlicedReductionWorkspaces.h
    Slicing.h
    ValidateRow.h
    WorkspaceNamesFactory.h

    AnalysisMode.h
    Experiment.h
    PolarizationCorrections.h
    RangeInLambda.h
    ReductionType.h
    RowTemplate.h
    SummationType.h
)

PREPEND( REDUCTION_SRC_FILES Reduction ${REDUCTION_SRC_FILES})
PREPEND( REDUCTION_INC_FILES Reduction ${REDUCTION_INC_FILES})

set ( REDUCTION_SRC_FILES ${REDUCTION_SRC_FILES} PARENT_SCOPE)
set ( REDUCTION_INC_FILES ${REDUCTION_INC_FILES} PARENT_SCOPE)
