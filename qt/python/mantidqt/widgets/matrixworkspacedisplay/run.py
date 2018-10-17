from mantidqt.widgets.matrixworkspacedisplay.presenter import MatrixWorkspaceDisplay
from mantid.simpleapi import Load
from qtpy.QtWidgets import QApplication

ws = Load(r'C:\Users\qbr77747\dev\m\workbench_matrixworkspace\test_masked_bins.nxs')

app = QApplication([])
window = MatrixWorkspaceDisplay(ws)
app.exec_()
