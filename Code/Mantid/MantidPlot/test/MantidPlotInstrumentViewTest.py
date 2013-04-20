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

class MantidPlotInstrumentViewTest(unittest.TestCase):
    
    def test_scale_type_can_be_changed(self):
        inst_win = getInstrumentView("loq_inst")
        render_tab = inst_win.getTab("Render")
        current_scale = render_tab.getScaleType()
        self.assertTrue(isinstance(current_scale, GraphOptions.ScaleType))
        render_tab.setScaleType(GraphOptions.Log10)
    
# Run the unit tests
mantidplottests.runTests(MantidPlotInstrumentViewTest)
