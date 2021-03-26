# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.Common.plot_widget.plotting_canvas.plot_color_queue import ColorQueue

EXAMPLE_COLOR_QUEUE = ["C" + str(i) for i in range(10)]


class PlotColorQueueTest(unittest.TestCase):
    def setUp(self):
        self.color_queue = ColorQueue(EXAMPLE_COLOR_QUEUE)

    def test_color_queue_returns_expected_colors_when_none_added(self):
        for i in range(10):
            self.assertEqual('C' + str(i), self.color_queue())

    def test_color_queue_repeats_cycle_when_out_of_range(self):
        for i in range(10):
            self.assertEqual('C' + str(i), self.color_queue())
        self.assertEqual('C0', self.color_queue())

    def test_color_queue_returns_when_colors_popped_and_pushed(self):
        # pop the first two colors
        self.color_queue()
        self.color_queue()
        self.color_queue()
        # Add first color back in, it should be top of queue
        self.color_queue += EXAMPLE_COLOR_QUEUE[0]

        self.assertEqual('C0', self.color_queue())
        # The next color in the cycle
        self.assertEqual('C3', self.color_queue())

    def test_add_unknown_color_to_queue_adds_at_back(self):
        self.color_queue += "test"
        self.assertEqual('test', self.color_queue._queue[-1][1])

    def test_add_color_ignores_duplicates(self):
        self.color_queue += EXAMPLE_COLOR_QUEUE[0]
        self.assertEqual('C0', self.color_queue())
        self.assertEqual('C1', self.color_queue())
        self.assertEqual('C9', self.color_queue._queue[-1][1])

    def test_that_reset_functions_correctly(self):
        self.color_queue()
        self.color_queue += 'C11'
        self.color_queue.reset()

        self.assertEqual('C0', self.color_queue())


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
