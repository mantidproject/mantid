# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

import unittest
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.widgets.plotconfigdialog.curvestabwidget.markertabwidget.view import MarkerTabWidgetView


@start_qapplication
class SampleTransmissionCalculatorViewTest(unittest.TestCase):
    def setUp(self):
        self.view = MarkerTabWidgetView()

    def test_marker_colour_disabled(self):
        self.view.set_colour_fields_enabled('None')
        self.assertFalse(self.view.face_color_selector_widget.isEnabled())
        self.assertFalse(self.view.edge_color_selector_widget.isEnabled())
        self.view.set_colour_fields_enabled('point')
        self.assertTrue(self.view.face_color_selector_widget.isEnabled())
        self.assertTrue(self.view.edge_color_selector_widget.isEnabled())


if __name__ == '__main__':
    unittest.main()
