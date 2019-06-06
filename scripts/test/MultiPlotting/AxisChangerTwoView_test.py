# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock

from MultiPlotting.AxisChanger.axis_changer_view import AxisChangerView
from Muon.GUI.Common.test_helpers import mock_widget


class AxisChangerTwoViewTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        label = "test"
        self.view = AxisChangerView(label)
        self.test_bounds = ["10", "20"]

        self.view.lower_bound.text = mock.Mock(
            return_value=self.test_bounds[0])
        self.view.upper_bound.text = mock.Mock(
            return_value=self.test_bounds[1])

        self.view.lower_bound.setText = mock.Mock()
        self.view.upper_bound.setText = mock.Mock()

        self.view.lower_bound.clear = mock.Mock()
        self.view.upper_bound.clear = mock.Mock()

        self.view.sig_bound_changed = mock.Mock()
        self.view.sig_bound_changed.emit = mock.Mock()
        self.view.sig_bound_changed.connect = mock.Mock()
        self.view.sig_bound_changed.disconnect = mock.Mock()

        self.slot = mock.Mock()

    def test_get_bounds(self):
        self.assertEqual(
            self.view.get_bounds(), [
                int(bound) for bound in self.test_bounds])

    def test_set_bounds(self):
        self.view.set_bounds(self.test_bounds)
        self.view.lower_bound.setText.assert_called_with(self.test_bounds[0])
        self.view.upper_bound.setText.assert_called_with(self.test_bounds[1])

    def test_clear_bounds(self):
        self.view.clear_bounds()
        self.assertEqual(self.view.lower_bound.clear.call_count, 1)
        self.assertEqual(self.view.upper_bound.clear.call_count, 1)

    def test_bound_changed(self):
        self.view._bound_changed()
        self.view.sig_bound_changed.emit.assert_called_with(
            list(self.view.get_bounds()))

    def test_on_bound_changed(self):
        self.view.on_bound_changed(self.slot)
        self.view.sig_bound_changed.connect.assert_called_with(self.slot)

    def test_unreg_on_bound_changed(self):
        self.view.unreg_bound_changed(self.slot)
        self.view.sig_bound_changed.disconnect.assert_called_with(
            self.slot)


if __name__ == "__main__":
    unittest.main()
