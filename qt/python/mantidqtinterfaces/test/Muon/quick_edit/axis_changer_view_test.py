# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.plot_widget.quick_edit.axis_changer.axis_changer_view import AxisChangerView

from mantidqt.utils.qt.testing import start_qapplication


@start_qapplication
class AxisChangerViewTest(unittest.TestCase):
    def setUp(self):
        self.view = AxisChangerView("X", None)
        self.view.set_limits([0.0, 10.0])
        self.slot = mock.Mock()
        self.view.on_range_changed(self.slot)

    def test_valid_range_is_emitted_and_stored(self):
        self.view.lower_bound.setText("1.0")
        self.view.upper_bound.setText("5.0")

        self.view._bound_changed()

        self.slot.assert_called_once_with([1.0, 5.0])
        self.assertEqual(self.view.get_limits(), [1.0, 5.0])

    def test_min_greater_than_max_is_rejected_and_reverted(self):
        self.view.lower_bound.setText("30.0")
        self.view.upper_bound.setText("10.0")

        self.view._bound_changed()

        self.slot.assert_not_called()
        self.assertEqual(self.view.get_limits(), [0.0, 10.0])

    def test_min_equal_to_max_is_rejected_and_reverted(self):
        self.view.lower_bound.setText("5.0")
        self.view.upper_bound.setText("5.0")

        self.view._bound_changed()

        self.slot.assert_not_called()
        self.assertEqual(self.view.get_limits(), [0.0, 10.0])

    def test_max_less_than_min_with_negative_values_is_rejected_and_reverted(self):
        self.view.lower_bound.setText("-5.0")
        self.view.upper_bound.setText("-10.0")

        self.view._bound_changed()

        self.slot.assert_not_called()
        self.assertEqual(self.view.get_limits(), [0.0, 10.0])

    def test_invalid_range_reverts_to_last_valid_range_not_initial_default(self):
        self.view.lower_bound.setText("1.0")
        self.view.upper_bound.setText("5.0")
        self.view._bound_changed()

        self.view.lower_bound.setText("30.0")
        self.view.upper_bound.setText("10.0")
        self.view._bound_changed()

        self.slot.assert_called_once_with([1.0, 5.0])
        self.assertEqual(self.view.get_limits(), [1.0, 5.0])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
