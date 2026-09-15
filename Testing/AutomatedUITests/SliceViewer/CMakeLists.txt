# Automated UI tests for SliceViewer, replacing dev-docs/source/Testing/SliceViewer/SliceViewer.rst.
#
# slice_viewer_gui_test_base.py is not listed: it holds the shared setup and the fixtures that build the guide's
# generated workspaces, not tests.
set(TEST_NAMES SliceViewerGuiCutViewerTest.py SliceViewerGuiMatrixWorkspaceTest.py SliceViewerGuiMdWorkspaceTest.py
               SliceViewerGuiObserverTest.py
)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.SliceViewer ${TEST_NAMES})
