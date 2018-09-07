import unittest

from Muon.GUI.ElementalAnalysis.Plotting.AxisChanger.axis_changer_view import AxisChangerView


from Muon.GUI.Common import mock_widget


try:
    from unittest import mock
except ImportError:
    import mock


class AxisChangerViewTest(unittest.TestCase):
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

        self.view.sig_upper_bound_changed = mock.Mock()
        self.view.sig_upper_bound_changed.emit = mock.Mock()
        self.view.sig_upper_bound_changed.connect = mock.Mock()
        self.view.sig_upper_bound_changed.disconnect = mock.Mock()

        self.view.sig_lower_bound_changed = mock.Mock()
        self.view.sig_lower_bound_changed.emit = mock.Mock()
        self.view.sig_lower_bound_changed.connect = mock.Mock()
        self.view.sig_lower_bound_changed.disconnect = mock.Mock()

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
        self.assertEquals(self.view.lower_bound.clear.call_count, 1)
        self.assertEquals(self.view.upper_bound.clear.call_count, 1)

    def test__lower_bound_changed(self):
        self.view._lower_bound_changed()
        self.view.sig_lower_bound_changed.emit.assert_called_with(
            self.view.get_bounds()[0])

    def test__upper_bound_changed(self):
        self.view._upper_bound_changed()
        self.view.sig_upper_bound_changed.emit.assert_called_with(
            self.view.get_bounds()[1])

    def test_on_lower_bound_changed(self):
        self.view.on_lower_bound_changed(self.slot)
        self.view.sig_lower_bound_changed.connect.assert_called_with(self.slot)

    def test_on_upper_bound_changed(self):
        self.view.on_upper_bound_changed(self.slot)
        self.view.sig_upper_bound_changed.connect.assert_called_with(self.slot)

    def test_unreg_on_lower_bound_changed(self):
        self.view.unreg_on_lower_bound_changed(self.slot)
        self.view.sig_lower_bound_changed.disconnect.assert_called_with(
            self.slot)

    def test_unreg_on_upper_bound_changed(self):
        self.view.unreg_on_upper_bound_changed(self.slot)
        self.view.sig_upper_bound_changed.disconnect.assert_called_with(
            self.slot)


if __name__ == "__main__":
    unittest.main()
