include(UiToPy)
add_subdirectory(batchwidget)

set(UI_FILES)

uitopy(UI_FILES CompileUIUIBase)

add_custom_target(CompileUIUI DEPENDS CompileUIUIBase CompileUIBatchWidgetInterface)

# Put all ui targets inside the 'CompilePyUI' folder or group in VS and the like, for convenience
set_property(TARGET CompileUIUIBase PROPERTY FOLDER "CompilePyUI")
