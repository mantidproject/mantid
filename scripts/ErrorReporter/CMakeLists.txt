include(UiToPy)

# List of UIs to Auto convert
set(UI_FILES
	errorreport.ui)

UiToPy(UI_FILES CompileUIErrorReporter)
