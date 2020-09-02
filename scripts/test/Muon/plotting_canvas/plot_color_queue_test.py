# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.plot_widget.plotting_canvas.plot_color_queue import ColorQueue


class PlottingCanvasPresenterTest(unittest.TestCase):

    def setUp(self):
        self.color_queue = ColorQueue()

    def test_color_queue_returns_expected_colors_when_none_added(self):
        for i in range(27):
            self.assertEqual('C' + str(i), self.color_queue())

    def test_color_queue_returns_top_queue_when_colors_added(self):
        self.color_queue += 'C5'
        self.color_queue += 'C7'

        self.assertEqual('C5', self.color_queue())
        self.assertEqual('C7', self.color_queue())
        self.assertEqual('C0', self.color_queue())

    def test_that_color_queue_ignores_invalid_additions(self):
        self.color_queue += 'fC5'
        self.color_queue += 'hello'

        self.assertEqual('C0', self.color_queue())

    def test_that_reset_functions_correctly(self):
        self.color_queue()
        self.color_queue += 'C11'
        self.color_queue.reset()

        self.assertEqual('C0', self.color_queue())


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
