# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from matplotlib.gridspec import GridSpec

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication

from MultiPlotting.multi_plotting_context import PlottingContext
from MultiPlotting.subplot.subplot import subplot


def rm_logic(name):
    if name == "two":
        return False
    return True


@start_qapplication
class SubplotTest(unittest.TestCase):
    def setUp(self):
        context = PlottingContext()
        self.subplot = subplot(context)
        self.subplot.canvas.draw = mock.MagicMock()

    def setup_rm(self):
        self.subplot._raise_rm_window = mock.Mock()
        self.subplot._raise_selector_window = mock.Mock()
        self.subplot._get_rm_window = mock.Mock()
        self.subplot._create_select_window = mock.MagicMock()

    def test_rm_one_plot_new_window(self):
        self.subplot._rm_window = None
        self.subplot._selector_window = None

        self.subplot._context.subplots["one"] = mock.Mock()
        self.setup_rm()
        self.subplot._rm()

        self.assertEqual(self.subplot._raise_rm_window.call_count, 0)
        self.assertEqual(self.subplot._raise_selector_window.call_count, 0)
        self.assertEqual(self.subplot._get_rm_window.call_count, 1)
        self.assertEqual(self.subplot._create_select_window.call_count, 0)

    def test_rm_one_plot_old_window(self):
        self.subplot._rm_window = mock.Mock()
        self.subplot._selector_window = None

        self.subplot._context.subplots["one"] = mock.Mock()
        self.setup_rm()
        self.subplot._rm()

        self.assertEqual(self.subplot._raise_rm_window.call_count, 0)
        self.assertEqual(self.subplot._raise_selector_window.call_count, 0)
        self.assertEqual(self.subplot._get_rm_window.call_count, 0)
        self.assertEqual(self.subplot._create_select_window.call_count, 0)

    def test_rm_two_plots_new_window(self):
        self.subplot._rm_window = None
        self.subplot._selector_window = None

        self.subplot._context.subplots["one"] = mock.Mock()
        self.subplot._context.subplots["two"] = mock.Mock()
        self.setup_rm()
        self.subplot._rm()

        self.assertEqual(self.subplot._raise_rm_window.call_count, 0)
        self.assertEqual(self.subplot._raise_selector_window.call_count, 0)
        self.assertEqual(self.subplot._get_rm_window.call_count, 0)
        self.assertEqual(self.subplot._create_select_window.call_count, 1)

    def test_rm_two_plots_old_select_window(self):
        self.subplot._rm_window = None
        self.subplot._selector_window = mock.Mock()

        self.subplot._context.subplots["one"] = mock.Mock()
        self.subplot._context.subplots["two"] = mock.Mock()
        self.setup_rm()
        self.subplot._rm()

        self.assertEqual(self.subplot._raise_rm_window.call_count, 0)
        self.assertEqual(self.subplot._raise_selector_window.call_count, 0)
        self.assertEqual(self.subplot._get_rm_window.call_count, 0)
        self.assertEqual(self.subplot._create_select_window.call_count, 1)

    def test_rm_two_plots_old_rm_window(self):
        self.subplot._rm_window = mock.Mock()
        self.subplot._selector_window = None

        self.subplot._context.subplots["one"] = mock.Mock()
        self.subplot._context.subplots["two"] = mock.Mock()
        self.setup_rm()
        self.subplot._rm()

        self.assertEqual(self.subplot._raise_rm_window.call_count, 0)
        self.assertEqual(self.subplot._raise_selector_window.call_count, 0)
        self.assertEqual(self.subplot._get_rm_window.call_count, 0)
        self.assertEqual(self.subplot._create_select_window.call_count, 1)

    def setup_apply_rm(self):
        self.subplot._rm_window = mock.Mock()
        self.subplot._rm_window.subplot = "test"
        self.subplot._context.subplots["test"] = mock.MagicMock()
        self.subplot._remove_subplot = mock.Mock()
        self.subplot._close_rm_window = mock.Mock()

    def test_apply_rmAll(self):
        names = ["one", "two", "three"]
        self.setup_apply_rm()
        self.subplot._rm_window.getState = mock.Mock(return_value=True)

        self.subplot._apply_rm(names)

        self.assertEqual(self.subplot._context.subplots["test"].removeLine.call_count, 3)
        self.assertEqual(self.subplot._close_rm_window.call_count, 1)

    def test_apply_rmNone(self):
        names = ["one", "two", "three"]
        self.setup_apply_rm()
        self.subplot._rm_window.getState = mock.Mock(return_value=False)

        self.subplot._apply_rm(names)

        self.assertEqual(self.subplot._context.subplots["test"].removeLine.call_count, 0)
        self.assertEqual(self.subplot._close_rm_window.call_count, 1)

    def test_apply_rmSome(self):
        names = ["one", "two", "three"]
        self.setup_apply_rm()
        self.subplot._rm_window.getState = mock.Mock(side_effect=rm_logic)

        self.subplot._apply_rm(names)

        self.assertEqual(self.subplot._context.subplots["test"].removeLine.call_count, 2)
        self.assertEqual(self.subplot._close_rm_window.call_count, 1)

    def test_addSubplot(self):
        self.subplot._update = mock.Mock()
        gridspec = GridSpec(2, 2)
        self.subplot._context.update_gridspec = mock.Mock()
        self.subplot._context._gridspec = gridspec

        self.subplot.add_subplot("test", 3)
        self.subplot._context.update_gridspec.assert_called_with(4)
        self.assertEqual(self.subplot._update.call_count, 1)

    def test_replaced_ws_false(self):
        one = mock.Mock()
        two = mock.Mock()
        self.subplot._context.subplots["one"] = one
        self.subplot._context.subplots["two"] = two
        self.subplot.canvas.draw = mock.Mock()
        ws = mock.Mock()
        self.subplot._context.subplots["one"].replace_ws = mock.Mock(return_value=False)
        self.subplot._context.subplots["two"].replace_ws = mock.Mock(return_value=False)

        self.subplot._replaced_ws(ws)
        self.assertEqual(self.subplot.canvas.draw.call_count, 0)

    def test_replaced_ws(self):
        one = mock.Mock()
        two = mock.Mock()
        self.subplot._context.subplots["one"] = one
        self.subplot._context.subplots["two"] = two
        self.subplot.canvas.draw = mock.Mock()
        ws = mock.Mock()
        self.subplot._context.subplots["one"].replace_ws = mock.Mock(return_value=False)
        self.subplot._context.subplots["two"].replace_ws = mock.Mock(return_value=True)

        self.subplot._replaced_ws(ws)
        self.assertEqual(self.subplot.canvas.draw.call_count, 1)

    def test_replaced_ws_true(self):
        one = mock.Mock()
        two = mock.Mock()
        self.subplot._context.subplots["one"] = one
        self.subplot._context.subplots["two"] = two
        self.subplot.canvas.draw = mock.Mock()
        ws = mock.Mock()
        self.subplot._context.subplots["one"].replace_ws = mock.Mock(return_value=True)
        self.subplot._context.subplots["two"].replace_ws = mock.Mock(return_value=True)

        self.subplot._replaced_ws(ws)
        self.assertEqual(self.subplot.canvas.draw.call_count, 2)

    def test_that_connect_rm_signal_calls_the_correct_function(self):
        self.subplot.sig_rm_line = mock.Mock()
        self.subplot.connect_rm_line_signal("slot value")

        self.subplot.sig_rm_line.connect.assert_called_with("slot value")

    def test_that_disconnect_rm_signal_calls_the_correct_function(self):
        self.subplot.sig_rm_line = mock.Mock()
        self.subplot.disconnect_rm_line_signal()

        self.assertEqual(1, self.subplot.sig_rm_line.disconnect.call_count)

    @staticmethod
    def rm_window_side_effect(name):
        return True

    def test_that_remove_line_calls_removeLine_the_correct_number_of_times(self):
        subplot_name = "subplot"
        lines = ["one", "two", "three"]
        calls = [mock.call("one"), mock.call("two"), mock.call("three")]
        self.subplot._context.subplots = {subplot_name: mock.Mock()}

        self.subplot.remove_lines(subplot_name, lines)

        self.subplot._context.subplots[subplot_name].removeLine.assert_has_calls(calls)

    def test_that_remove_lines_removes_subplot_is_no_line_is_present(self):
        subplot_name = "subplot"
        self.subplot._context.get_lines = mock.Mock(return_value=[])
        self.subplot._context.subplots = {subplot_name: mock.Mock()}
        self.subplot._remove_subplot = mock.Mock()
        self.subplot.canvas = mock.Mock()

        self.subplot.remove_lines(subplot_name, ["one"])

        self.assertEqual(1, self.subplot._remove_subplot.call_count)
        self.assertEqual(0, self.subplot.canvas.draw.call_count)

    def test_that_remove_lines_updates_canvas_without_closing_plot_if_lines_are_present(self):
        subplot_name = "subplot"
        self.subplot._context.get_lines = mock.Mock(return_value=["not empty"])
        self.subplot._context.subplots = {subplot_name: mock.Mock()}
        self.subplot._remove_subplot = mock.Mock()
        self.subplot.canvas = mock.Mock()

        self.subplot.remove_lines(subplot_name, ["one"])

        self.assertEqual(0, self.subplot._remove_subplot.call_count)
        self.assertEqual(1, self.subplot.canvas.draw.call_count)

    def test_that_remove_lines_emits_signal(self):
        subplot_name = "subplot"
        self.subplot._context.get_lines = mock.Mock(return_value=["not empty"])
        self.subplot._context.subplots = {subplot_name: mock.Mock()}
        self.subplot.sig_rm_line = mock.Mock()
        self.subplot.canvas = mock.Mock()

        self.subplot.remove_lines(subplot_name, ["one"])

        self.assertEqual(1, self.subplot.sig_rm_line.emit.call_count)


if __name__ == "__main__":
    unittest.main()
