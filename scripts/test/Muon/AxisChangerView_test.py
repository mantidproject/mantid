<<<<<<< 849aea4b1c8c90117a4aa34bc0a2ec41c2080560
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

        self.view.sig_bounds_changed = mock.Mock()
        self.view.sig_bounds_changed.emit = mock.Mock()
        self.view.sig_bounds_changed.connect = mock.Mock()
        self.view.sig_bounds_changed.disconnect = mock.Mock()

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

    def test__bounds_changed(self):
        self.view._bounds_changed()
        self.view.sig_bounds_changed.emit.assert_called_with(
            self.view.get_bounds())

    def test_on_bounds_changed(self):
        self.view.on_bounds_changed(self.slot)
        self.view.sig_bounds_changed.connect.assert_called_with(self.slot)

    def test_unreg_on_bounds_changed(self):
        self.view.unreg_on_bounds_changed(self.slot)
        self.view.sig_bounds_changed.disconnect.assert_called_with(self.slot)


if __name__ == "__main__":
    unittest.main()
||||||| merged common ancestors
=======
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

        self.view.sig_bounds_changed = mock.Mock()
        self.view.sig_bounds_changed.emit = mock.Mock()
        self.view.sig_bounds_changed.connect = mock.Mock()
        self.view.sig_bounds_changed.disconnect = mock.Mock()

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

    def test__bounds_changed(self):
        self.view._bounds_changed()
        self.view.sig_bounds_changed.emit.assert_called_with(
            self.view.get_bounds())

    def test_on_bounds_changed(self):
        self.view.on_bounds_changed(self.slot)
        self.view.sig_bounds_changed.connect.assert_called_with(self.slot)

    def test_unreg_on_bounds_changed(self):
        self.view.unreg_on_bounds_changed(self.slot)
        self.view.sig_bounds_changed.disconnect.assert_called_with(self.slot)


if __name__ == "__main__":
    unittest.main()
>>>>>>> refs #23260 add axis_changer_presenter tests
