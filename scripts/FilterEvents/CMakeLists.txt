include(UiToPy)

# List of UIs to Auto convert
set( UI_FILES
  ErrorMessage.ui
  MainWindow.ui 
)

UiToPy( UI_FILES CompileUIFilterEvents)


