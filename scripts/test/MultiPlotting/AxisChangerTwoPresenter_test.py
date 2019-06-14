# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from MultiPlotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from MultiPlotting.AxisChanger.axis_changer_view import AxisChangerView


class AxisChangerTwoPresenterTest(unittest.TestCase):
    def setUp(self):
        view = mock.create_autospec(AxisChangerView)
        self.acp = AxisChangerPresenter(view)
        self.view = self.acp.view
        self.slot = mock.Mock()
        self.bounds = mock.Mock()

    def test_get_bounds(self):
        self.acp.get_bounds()
        self.assertEqual(self.view.get_bounds.call_count, 1)

    def test_set_bounds(self):
        self.acp.set_bounds(self.bounds)
        self.view.set_bounds.assert_called_with(self.bounds)

    def test_clear_bounds(self):
        self.acp.clear_bounds()
        self.assertEqual(self.view.clear_bounds.call_count, 1)

    def test_on_lower_bound_changed(self):
        self.acp.on_bound_changed(self.slot)
        self.view.on_bound_changed.assert_called_with(self.slot)

    def test_unreg_bound_changed(self):
        self.acp.unreg_on_bound_changed(self.slot)
        self.view.unreg_bound_changed.assert_called_with(self.slot)


if __name__ == "__main__":
    unittest.main()
