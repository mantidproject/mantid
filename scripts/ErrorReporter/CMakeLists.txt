include(UiToPy)

# List of UIs to Auto convert
set(UI_FILES
	errorreport.ui)

uitopy(UI_FILES CompileUIErrorReporter)
