include(UiToPy)

# List of UIs to Auto convert
set( UI_FILES
  MainWindow.ui 
  View3DWidget.ui
  OptimizeLattice.ui
  RefineUbFftDialog.ui
  SpiceViewerDialog.ui
)

UiToPy( UI_FILES CompileUIHFIR_4Circle_Reduction)
