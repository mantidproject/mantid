# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from qtpy.QtGui import QCloseEvent as event
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.elemental_analysis import ElementalAnalysisGui


@start_qapplication
class ElementalAnalysisGUITest(unittest.TestCase):
    def test_start_and_close(self):
        GUI = ElementalAnalysisGui()
        self.assertTrue(GUI is not None)
        try:
            GUI.closeEvent(event())
        except:
            self.assertEqual("GUI did not close correctly", "")


if __name__ == '__main__':
    unittest.main()
