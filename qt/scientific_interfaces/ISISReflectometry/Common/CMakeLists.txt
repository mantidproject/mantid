set(COMMON_SRC_FILES Detector.cpp Parse.cpp GetInstrumentParameter.cpp InstrumentParameters.cpp OptionDefaults.cpp
                     Clipboard.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio IMPORTANT: Include files are required in
# the MOC_FILES set. Scroll down to find it.
set(COMMON_INC_FILES
    Clipboard.h
    Detector.h
    DllConfig.h
    First.h
    GetInstrumentParameter.h
    IndexOf.h
    InstrumentParameters.h
    Map.h
    OptionDefaults.h
    Parse.h
    QWidgetGroup.h
    ValidationResult.h
    ZipRange.h
)

prepend(COMMON_SRC_FILES Common ${COMMON_SRC_FILES})
prepend(COMMON_INC_FILES Common ${COMMON_INC_FILES})

set(COMMON_SRC_FILES
    ${COMMON_SRC_FILES}
    PARENT_SCOPE
)
set(COMMON_INC_FILES
    ${COMMON_INC_FILES}
    PARENT_SCOPE
)
