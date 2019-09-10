set(REDUCTION_SRC_FILES
    Group.cpp
    Item.cpp
    ItemState.cpp
    RangeInQ.cpp
    ReductionWorkspaces.cpp
    Row.cpp
    RowLocation.cpp
    ReductionJobs.cpp
    ValidateRow.cpp
    Slicing.cpp
    ParseReflectometryStrings.cpp
    TransmissionRunPair.cpp
    TransmissionStitchOptions.cpp
    Batch.cpp
    DetectorCorrections.cpp
    Experiment.cpp
    FloodCorrections.cpp
    Instrument.cpp
    MonitorCorrections.cpp
    PolarizationCorrections.cpp
    RangeInLambda.cpp
    RunsTable.cpp
    PerThetaDefaults.cpp
    ValidatePerThetaDefaults.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(REDUCTION_INC_FILES
    Group.h
    Item.h
    ItemState.h
    RangeInQ.h
    ReductionJobs.h
    ReductionWorkspaces.h
    Row.h
    RowLocation.h
    Slicing.h
    ValidateRow.h
    ParseReflectometryStrings.h
    ProcessingInstructions.h
    TransmissionRunPair.h
    TransmissionStitchOptions.h
    ValidatePerThetaDefaults.h
    AnalysisMode.h
    Batch.h
    DetectorCorrections.h
    Experiment.h
    FloodCorrections.h
    Instrument.h
    MonitorCorrections.h
    PolarizationCorrections.h
    RangeInLambda.h
    ReductionType.h
    RunsTable.h
    PerThetaDefaults.h
    SummationType.h)

prepend(REDUCTION_SRC_FILES Reduction ${REDUCTION_SRC_FILES})
prepend(REDUCTION_INC_FILES Reduction ${REDUCTION_INC_FILES})

set(REDUCTION_SRC_FILES ${REDUCTION_SRC_FILES} PARENT_SCOPE)
set(REDUCTION_INC_FILES ${REDUCTION_INC_FILES} PARENT_SCOPE)
