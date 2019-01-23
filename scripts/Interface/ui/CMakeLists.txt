include(UiToPy)
add_subdirectory(reflectometer)
add_subdirectory(sans_isis)
add_subdirectory(dataprocessorinterface)
add_subdirectory(batchwidget)
add_subdirectory(poldi)

SET(UI_FILES
stitcher.ui
)

UiToPy(UI_FILES CompileUIUIBase)

add_custom_target(CompileUIUI DEPENDS
  CompileUIUIBase
  CompileUIReflectometer
  CompileUISANSDataProcessorInterface
  CompileUIDataProcessorInterface
  CompileUIBatchWidgetInterface
  CompileUIPoldi
)

# Put all ui targets inside the 'CompilePyUI' folder or group in VS and the like, for convenience
set_property ( TARGET CompileUIUIBase PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIReflectometer PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIDataProcessorInterface PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUISANSDataProcessorInterface PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIPoldi PROPERTY FOLDER "CompilePyUI" )
