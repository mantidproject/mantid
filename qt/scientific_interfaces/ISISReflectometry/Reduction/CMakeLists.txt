set ( REDUCTION_SRC_FILES
    Group.cpp
    RangeInQ.cpp
    ReductionWorkspaces.cpp
    Row.cpp
    ReductionJobs.cpp
    ValidateRow.cpp
    Slicing.cpp
    ParseReflectometryStrings.cpp

    DetectorCorrections.cpp
    Experiment.cpp
    Instrument.cpp
    MonitorCorrections.cpp
    PolarizationCorrections.cpp
    RangeInLambda.cpp
    PerThetaDefaults.cpp
    ValidatePerThetaDefaults.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( REDUCTION_INC_FILES
    Group.h
    RangeInQ.h
    ReductionJobs.h
    ReductionWorkspaces.h
    Row.h
    Slicing.h
    ValidateRow.h
    ParseReflectometryStrings.h
    ValidatePerThetaDefaults.h

    AnalysisMode.h
    DetectorCorrections.h
    Experiment.h
    Instrument.h
    MonitorCorrections.h
    PolarizationCorrections.h
    RangeInLambda.h
    ReductionType.h
    PerThetaDefaults.h
    SummationType.h
)

PREPEND( REDUCTION_SRC_FILES Reduction ${REDUCTION_SRC_FILES})
PREPEND( REDUCTION_INC_FILES Reduction ${REDUCTION_INC_FILES})

set ( REDUCTION_SRC_FILES ${REDUCTION_SRC_FILES} PARENT_SCOPE)
set ( REDUCTION_INC_FILES ${REDUCTION_INC_FILES} PARENT_SCOPE)
