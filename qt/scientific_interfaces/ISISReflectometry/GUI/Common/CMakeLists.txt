set(COMMON_SRC_FILES Plotter.cpp Encoder.cpp Decoder.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
set(COMMON_INC_FILES
    IFileHandler.h
    IJobManager.h
    IReflMessageHandler.h
    IPythonRunner.h
    IPlotter.h
    Plotter.h
    Encoder.h
    Decoder.h
)

prepend(COMMON_SRC_FILES GUI/Common ${COMMON_SRC_FILES})
prepend(COMMON_INC_FILES GUI/Common ${COMMON_INC_FILES})

set(COMMON_SRC_FILES
    ${COMMON_SRC_FILES}
    PARENT_SCOPE
)
set(COMMON_INC_FILES
    ${COMMON_INC_FILES}
    PARENT_SCOPE
)
