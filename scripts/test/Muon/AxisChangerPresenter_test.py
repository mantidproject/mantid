import unittest

from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_presenter import AxisChangerPresenter
from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_view import AxisChangerView


try:
    from unittest import mock
except ImportError:
    import mock


class AxisChangerPresenterTest(unittest.TestCase):
    def setUp(self):
        view = mock.create_autospec(AxisChangerView)
        self.acp = AxisChangerPresenter(view)
        self.view = self.acp.view
        self.slot = mock.Mock()
        self.bounds = mock.Mock()

    def test_get_bounds(self):
        self.acp.get_bounds()
        self.assertEquals(self.view.get_bounds.call_count, 1)

    def test_set_bounds(self):
        self.acp.set_bounds(self.bounds)
        self.view.set_bounds.assert_called_with(self.bounds)

    def test_clear_bounds(self):
        self.acp.clear_bounds()
        self.assertEquals(self.view.clear_bounds.call_count, 1)

    def test_on_bounds_changed(self):
        self.acp.on_bounds_changed(self.slot)
        self.view.on_bounds_changed.assert_called_with(self.slot)

    def test_unreg_on_bounds_changed(self):
        self.acp.unreg_on_bounds_changed(self.slot)
        self.view.unreg_on_bounds_changed.assert_called_with(self.slot)


if __name__ == "__main__":
    unittest.main()
