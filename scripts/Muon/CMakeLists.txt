include (UiToPy)

# List of UIs to Auto convert
set( UI_FILES
)

UiToPy( UI_FILES CompileUIFrequency_Domain_Analysis)

find_program(PIP "pip")
if (PIP)
	set(REQ	"${CMAKE_CURRENT_SOURCE_DIR}/GUI/ElementalAnalysis/requirements.txt")
	execute_process(COMMAND ${PIP} install --user -r ${REQ})
endif()
