# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_model import PlottingCanvasModel


class PlottingCanvasModelTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.Mock()
        self.model = PlottingCanvasModel(self.context)

    def test_create_plot_information_returns_expected_information_for_single_plot(self):
        pass

    def test_create_plot_information_returns_expected_information_for_tiled_plot(self):
        pass


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
