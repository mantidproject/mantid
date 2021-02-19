# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from Muon.GUI.Common.contexts.plot_edit_context import PlotEditContext


class MuonPlotEidtContextTest(unittest.TestCase):
    def setUp(self):

        self.context = PlotEditContext()

    def tearDown(self):
        return

    def test_get_xlim(self):
        limits = self.context.get_xlim
        self.assertEqual([0.0, 15.0], limits)

    def test_update_xlim(self):
        self.context.update_xlim([3.0, 26.0])
        limits = self.context.get_xlim
        self.assertEqual([3.0, 26.0], limits)

    def test_get_ylim(self):
        limits = self.context.get_ylim
        self.assertEqual([-0.3, 0.3], limits)

    def test_update_ylim(self):
        self.context.update_ylim([-3.0, 2.6])
        limits = self.context.get_ylim
        self.assertEqual([-3.0, 2.6], limits)

    def test_get_error_state(self):
        self.assertEqual(False, self.context.get_error_state)

    def test_update_error_state(self):
        self.context.update_error_state(True)
        self.assertEqual(True, self.context.get_error_state)

    def test_get_autoscale_state(self):
        self.assertEqual(False, self.context.get_autoscale_state)

    def test_update_autoscale_state(self):
        self.context.update_autoscale_state(True)
        self.assertEqual(True, self.context.get_autoscale_state)

    def test_get_axis(self):
        self.assertEqual(0, self.context.axis)

    def test_set_axis(self):
        self.context.set_axis(4)
        self.assertEqual(4, self.context.axis)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
