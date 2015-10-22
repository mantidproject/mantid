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
