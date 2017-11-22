include(UiToPy)

# List of UIs to Auto convert
set( UI_FILES
  MainWindow.ui
  messagebox.ui
  View3DWidget.ui
  OptimizeLattice.ui
  RefineUbFftDialog.ui
  SpiceViewerDialog.ui
  UBSelectPeaksDialog.ui
  AddUBPeaksDialog.ui
  PeakIntegrationSpreadSheet.ui
  httpserversetup.ui
  preprocess_window.ui
  peak_integration_info.ui
)

UiToPy( UI_FILES CompileUIHFIR_4Circle_Reduction)
