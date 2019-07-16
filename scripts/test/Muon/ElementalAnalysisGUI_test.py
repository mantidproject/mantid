# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

import unittest


from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from MultiPlotting.label import Label
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui


class ElementalAnalysisGUITest(GuiTest):
    def setUp(self):
        self.GUI = ElementalAnalysisGui()

    def test_get_color(self):
        self.GUI.color_index = 0
        self.GUI.get_color()
        self.assertEquals(1, self.GUI.color_index)

    def test_gen_label_no_element(self):
        name = "string"
        x_value_in = 1.0
        gen_label_output = self.GUI._gen_label(name, x_value_in)
        self.assertEquals(None, gen_label_output)

    def test_gen_label_x_value_not_float(self):
        name = "string"
        x_value_in = "string"
        self.GUI.element_lines["H"] = []
        gen_label_output = self.GUI._gen_label(name, x_value_in, element="H")
        self.assertEquals(None, gen_label_output)

    def test_gen_label_output_is_label(self):
        name = "string"
        x_value_in = 1.0
        self.GUI.element_lines["H"] = []
        gen_label_output = self.GUI._gen_label(name, x_value_in, element="H")
        self.assertEquals(Label, type(gen_label_output))

    def test_plot_line(self):
        self.GUI._gen_label = mock.Mock()
        self.GUI._plot_line_once = mock.Mock()
        self.GUI.plotwindow = None
        name = "string"
        x_value_in = 1.0
        color = "C0"
        self.assertEqual(None, self.GUI._plot_line(name, x_value_in, color))


if __name__ == "__main__":
    unittest.main()
