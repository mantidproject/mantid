include(UiToPy)
add_subdirectory(diffraction)
add_subdirectory(inelastic)
add_subdirectory(reflectometer)
add_subdirectory(sans)

SET(UI_FILES
cluster_details_dialog.ui
cluster_status.ui
data_catalog.ui
hfir_output.ui
instrument_dialog.ui
reduction_main.ui
stitcher.ui
trans_direct_beam.ui
trans_spreader.ui
)

UiToPy(UI_FILES CompileUIUIBase)

add_custom_target(CompileUIUI DEPENDS
  CompileUIUIBase
  CompileUIDiffraction
  CompileUIInElastic
  CompileUIReflectometer
  CompileUISANS
)

# Put all ui targets inside the 'CompilePyUI' folder or group in VS and the like, for convenience
set_property ( TARGET CompileUIUIBase PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIDiffraction PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIInElastic PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIReflectometer PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUISANS PROPERTY FOLDER "CompilePyUI" )
