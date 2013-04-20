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

# We need an instrument to test
LoadEmptyInstrument(Filename="LOQ_Definition_20121016-.xml", 
                    OutputWorkspace="loq_inst")
INST_WIN = getInstrumentView("loq_inst")

class MantidPlotInstrumentViewTest(unittest.TestCase):

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

    def test_closing_window_invalidates_reference(self):
        inst = getInstrumentView("loq_inst")
        render_tab = inst.getTab("Render")
        inst.close()
        self.assertTrue(inst._getHeldObject() is None)
    
# Run the unit tests
mantidplottests.runTests(MantidPlotInstrumentViewTest)
