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
from collections import namedtuple


@start_qapplication
class MarkerTabWidgetViewTest(unittest.TestCase):
    def setUp(self):
        self.view = MarkerTabWidgetView()

    def test_marker_colour_disabled(self):
        self.view.set_colour_fields_enabled('None')
        self.assertFalse(self.view.face_color_selector_widget.isEnabled())
        self.assertFalse(self.view.edge_color_selector_widget.isEnabled())
        self.view.set_colour_fields_enabled('point')
        self.assertTrue(self.view.face_color_selector_widget.isEnabled())
        self.assertTrue(self.view.edge_color_selector_widget.isEnabled())

    def test_update_fields_applies_marker_settings_correctly(self):
        CurveProps = namedtuple("CurveProps", ['marker', 'markersize', 'markerfacecolor', 'markeredgecolor'])
        fake_curve_props = CurveProps(marker = 'circle', markersize = 6.0,
                                      markerfacecolor = '#1f77b4',
                                      markeredgecolor = '#1f77b4')

        self.view.update_fields(fake_curve_props)
        self.assertTrue(self.view.get_style() == 'circle')
        self.assertTrue(self.view.face_color_selector_widget.isEnabled())
        self.assertTrue(self.view.edge_color_selector_widget.isEnabled())


if __name__ == '__main__':
    unittest.main()
