# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqtinterfaces.MultiPlotting.multi_plotting_context import PlottingContext
from mantidqtinterfaces.MultiPlotting.subplot.subplot_context import subplotContext
from line_helper import line


class gen_ws(object):
    def __init__(self, mock):
        self._input = "in"
        self._OutputWorkspace = mock

    def __len__(self):
        return 2

    @property
    def OutputWorkspace(self):
        return self._OutputWorkspace


class MultiPlottingContextTest(unittest.TestCase):
    def setUp(self):
        self.context = PlottingContext()

    def test_add_line_1(self):
        spec_num = 4
        ws = mock.MagicMock()
        # add mock subplot
        subplot = mock.MagicMock()
        self.subplot = mock.create_autospec(subplotContext, instance=True)
        with mock.patch("mantidqtinterfaces.MultiPlotting.subplot.subplot_context.subplotContext.addLine") as patch:
            self.context.addSubplot("one", subplot)
            self.context.addLine("one", ws, spec_num, "C0")
            self.assertEqual(patch.call_count, 1)
            patch.assert_called_with(ws, spec_num, color="C0")

    def test_add_line_2(self):
        spec_num = 4
        mock_ws = mock.MagicMock()
        ws = gen_ws(mock_ws)
        # add mock subplot
        subplot = mock.MagicMock()
        self.subplot = mock.create_autospec(subplotContext, instance=True)
        with mock.patch("mantidqtinterfaces.MultiPlotting.subplot.subplot_context.subplotContext.addLine") as patch:
            self.context.addSubplot("one", subplot)
            self.context.addLine("one", ws, spec_num, "C0")
            self.assertEqual(patch.call_count, 1)
            patch.assert_called_with(mock_ws, spec_num, color="C0")

    def test_update_layout(self):
        # add mocks
        figure = mock.Mock()
        self.subplot = mock.create_autospec(subplotContext, instance=True)
        names = ["one", "two", "three"]
        for name in names:
            self.context.addSubplot(name, mock.Mock())

        gridspec = mock.Mock()
        self.context._gridspec = gridspec
        with mock.patch("mantidqtinterfaces.MultiPlotting.subplot.subplot_context.subplotContext.update_gridspec") as patch:
            self.context.update_layout(figure)
            self.assertEqual(patch.call_count, 3)
            # only last iteration survives
            patch.assert_called_with(gridspec, figure, 2)

    def test_subplot_empty_true(self):
        names = ["one", "two", "three"]
        for name in names:
            self.context.addSubplot(name, mock.Mock())

        for name in names:
            self.assertEqual(self.context.is_subplot_empty(name), True)

    def test_subplot_empty_false(self):
        names = ["one", "two", "three"]

        no_lines = 1

        ws = mock.MagicMock()
        with mock.patch("mantid.plots.axesfunctions.plot") as patch:
            patch.return_value = tuple([line()])

            for name in names:
                self.context.addSubplot(name, mock.Mock())
                for k in range(0, no_lines):
                    self.context.addLine(name, ws, 1, "C0")
                no_lines += 1

        for name in names:
            self.assertEqual(self.context.is_subplot_empty(name), False)

    def test_that_remove_line_does_nothing_if_given_bad_subplot_name(self):
        self.context.subplots = {"plot": mock.Mock()}

        self.context.remove_line("plot that does not exist", "one")

        self.assertEqual(self.context.subplots["plot"].remove_line.call_count, 0)
        self.assertEqual(self.context.subplots["plot"].redraw_annotations.call_count, 0)

    def test_that_remove_line_calls_the_correct_functions(self):
        self.context.subplots = {"plot": mock.Mock()}

        self.context.remove_line("plot", "line name")

        self.assertEqual(1, self.context.subplots["plot"].removeLine.call_count)
        self.assertEqual(0, self.context.subplots["plot"].redraw_annotations.call_count)
        self.context.subplots["plot"].removeLine.assert_called_with("line name")

    def test_that_get_lines_returns_empty_list_if_given_bad_name(self):
        self.context.subplots = {"plot": mock.Mock()}

        lines = self.context.get_lines("not a valid plot")

        self.assertEqual(lines, [])

    def test_that_get_lines_returns_correct_lines(self):
        self.context.subplots = {"plot": mock.Mock()}
        self.context.subplots["plot"].lines = ["one", "two", "three"]

        lines = self.context.get_lines("plot")

        self.assertEqual(["one", "two", "three"], lines)


if __name__ == "__main__":
    unittest.main()
