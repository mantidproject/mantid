"""
Test the interaction with the instrument view.
Assertion that things work is difficult so mosts
test will just that things don't crash.
"""
import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtGui, QtCore

# ===== Test workspace ======
X1 = np.linspace(0,10, 100)
Y1 = 1000*(np.sin(X1)**2) + X1*10
X1 = np.append(X1, 10.1)

X2 = np.linspace(2,12, 100)
Y2 = 500*(np.cos(X2/2.)**2) + 20
X2 = np.append(X2, 12.10)

X = np.append(X1, X2)
Y = np.append(Y1, Y2)
E = np.sqrt(Y)

CreateWorkspace(OutputWorkspace="loq_inst", DataX=list(X), DataY=list(Y),
                DataE=list(E), NSpec=2, UnitX="TOF", YUnitLabel="Counts",
                WorkspaceTitle="Faked data Workspace")
LoadInstrument(Workspace="loq_inst", InstrumentName="LOQ")
INST_WIN = getInstrumentView("loq_inst")

class MantidPlotInstrumentViewTest(unittest.TestCase):

    def test_invalid_tab_title_raises_exception(self):
        self.assertRaises(ValueError, INST_WIN.getTab, "wont see this tab")

    def test_get_tab_can_use_title_index_or_enum(self):
        render_tab = INST_WIN.getTab("Render")
        self.assertTrue(render_tab is not None)
        render_tab = INST_WIN.getTab(InstrumentWindow.RENDER)
        self.assertTrue(render_tab is not None)
        render_tab = INST_WIN.getTab(0)
        self.assertTrue(render_tab is not None)

    def test_integration_range_can_be_changed(self):
        INST_WIN.setBinRange(5,10)

    def test_scale_type_can_be_changed(self):
        render_tab = INST_WIN.getTab("Render")
        current_scale = render_tab.getScaleType()
        self.assertTrue(isinstance(current_scale, GraphOptions.ScaleType))
        render_tab.setScaleType(GraphOptions.Log10)

    def test_colour_range_can_be_changed(self):
        render_tab = INST_WIN.getTab("Render")
        render_tab.setMinValue(1.25)
        render_tab.setMaxValue(1.75)
        render_tab.setRange(1.35,1.85)

    def test_display_options(self):
        render_tab = INST_WIN.getTab("Render")
        render_tab.showAxes(True)
        render_tab.displayDetectorsOnly(True)
        render_tab.setColorMapAutoscaling(False)
        render_tab.setSurfaceType(0)
        render_tab.setAxis("Y-")
        render_tab.flipUnwrappedView(True)

    def test_closing_window_invalidates_reference(self):
        inst = getInstrumentView("loq_inst")
        inst.close()
        self.assertTrue(inst._getHeldObject() is None)

# Run the unit tests
mantidplottests.runTests(MantidPlotInstrumentViewTest)
