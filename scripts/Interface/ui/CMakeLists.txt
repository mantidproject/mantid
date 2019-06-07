include(UiToPy)
add_subdirectory(dataprocessorinterface)
add_subdirectory(batchwidget)
add_subdirectory(poldi)
add_subdirectory(ill)

set(UI_FILES
    stitcher.ui)

UiToPy(UI_FILES CompileUIUIBase)

add_custom_target(CompileUIUI
                  DEPENDS CompileUIUIBase
                          CompileUIDataProcessorInterface
                          CompileUIBatchWidgetInterface
                          CompileUIPoldi
                          CompileUIILL)

# Put all ui targets inside the 'CompilePyUI' folder or group in VS and the
# like, for convenience
set_property(TARGET CompileUIUIBase PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIDataProcessorInterface
             PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIPoldi PROPERTY FOLDER "CompilePyUI")
set_property(TARGET CompileUIILL PROPERTY FOLDER "ill")
