# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication

from MultiPlotting.multi_plotting_context import PlottingContext
from MultiPlotting.multi_plotting_widget import MultiPlotWidget


class bounds(object):
    def __init__(self,x,y):
        self.x = x
        self.y =y
        self.error = False

    @property
    def xbounds(self):
       return self.x

    @property
    def ybounds(self):
       return self.y

    @property
    def errors(self):
        return self.error


def data():
    values = {}
    values["one"] = bounds([5,20],[5,10])
    values["two"] = bounds([6,10],[0,9])
    values["three"] = bounds([-1,11],[7,8])
    values["four"] = bounds([4,12],[4,50])
    return values


@start_qapplication
class MultiPlotWidgetTest(unittest.TestCase):

    def setUp(self):
        context = PlottingContext()
        self.widget = MultiPlotWidget(context)

    def test_add_subplot(self):
        with mock.patch("MultiPlotting.QuickEdit.quickEdit_widget.QuickEditWidget.add_subplot") as qe_patch:
            self.widget.add_subplot("test")
            self.assertEqual(qe_patch.call_count,1)

    def test_plot(self):
        with mock.patch("MultiPlotting.subplot.subplot.subplot.plot") as patch:
             ws = mock.MagicMock()
             subplotName = "test"
             specNum = 4
             self.widget.plot(subplotName, ws, specNum)
             patch.assert_called_with(subplotName, ws, specNum=specNum)
             self.assertEqual(patch.call_count,1)

    def test_setAllValues(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value = list(data().keys()))
        self.widget._x_range_changed = mock.MagicMock()
        self.widget._y_range_changed = mock.MagicMock()
        self.widget._check_all_errors = mock.MagicMock(return_value = False)
        self.widget._change_errors = mock.MagicMock()

        self.widget.set_all_values()
        self.widget._x_range_changed.assert_called_with([-1,20])
        self.widget._y_range_changed.assert_called_with([0,50])

    def test_updateQuickEditNoMatch(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value = data())
        self.widget.quickEdit.rm_subplot = mock.Mock()
        self.widget.quickEdit._if_empty_close = mock.Mock()

        self.widget._update_quick_edit("no match")
        self.assertEqual(self.widget.quickEdit.rm_subplot.call_count, 1)

    def test_updateQuickEdit1Match(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =["two"])
        self.widget.quickEdit.set_plot_x_range = mock.MagicMock()
        self.widget.quickEdit.set_plot_y_range = mock.MagicMock()

        self.widget._update_quick_edit("two")
        self.widget.quickEdit.set_plot_x_range.assert_called_with([6,10])
        self.widget.quickEdit.set_plot_y_range.assert_called_with([0,9])

    def test_updateQuickEdit1NoMatch(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =["two"])
        self.widget.quickEdit.set_plot_x_range = mock.MagicMock()
        self.widget.quickEdit.set_plot_y_range = mock.MagicMock()

        self.widget._update_quick_edit("three")
        self.assertEqual(self.widget.quickEdit.set_plot_x_range.call_count,0)
        self.assertEqual(self.widget.quickEdit.set_plot_y_range.call_count,0)

    def test_updateQuickEditMany(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =["two","three"])
        self.widget.quickEdit.set_plot_x_range = mock.MagicMock()
        self.widget.quickEdit.set_plot_y_range = mock.MagicMock()

        self.widget._update_quick_edit("two")
        self.widget.quickEdit.set_plot_x_range.assert_called_with([6,10])
        self.widget.quickEdit.set_plot_y_range.assert_called_with([0,9])

    def test_selectionChanged(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =["two"])
        self.widget._check_all_errors = mock.MagicMock(return_value = False)
        self.widget._change_errors = mock.MagicMock()
        self.widget.quickEdit.set_plot_x_range = mock.MagicMock()
        self.widget.quickEdit.set_plot_y_range = mock.MagicMock()

        self.widget._selection_changed(1)
        self.widget.quickEdit.set_plot_x_range.assert_called_with([6,10])
        self.widget.quickEdit.set_plot_y_range.assert_called_with([0,9])

    def test_selectionChangedAll(self):
        self.widget._context.subplots = data()
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =["two","three"])
        xbounds = [-1,2]
        ybounds = [-10,20]
        self.widget._context.get_xBounds = mock.MagicMock(return_value = xbounds)
        self.widget._context.get_yBounds = mock.MagicMock(return_value = ybounds)
        self.widget._check_all_errors = mock.MagicMock(return_value = False)
        self.widget._change_errors = mock.MagicMock()
        self.widget.quickEdit.set_plot_x_range = mock.MagicMock()
        self.widget.quickEdit.set_plot_y_range = mock.MagicMock()
        self.widget._x_range_changed = mock.MagicMock()
        self.widget._y_range_changed = mock.MagicMock()

        self.widget._selection_changed(1)
        self.widget.quickEdit.set_plot_x_range.assert_called_with(xbounds)
        self.widget.quickEdit.set_plot_y_range.assert_called_with(ybounds)
        self.widget._x_range_changed.assert_called_with(xbounds)
        self.widget._y_range_changed.assert_called_with(ybounds)

    def test_xRangeChanged(self):
        names = ["two","three"]
        xbounds = [9,18]
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =names)
        self.widget._context.set_xBounds = mock.MagicMock()
        self.widget.plots.set_plot_x_range = mock.MagicMock()
        self.widget._x_range_changed(xbounds)
        self.widget._context.set_xBounds.assert_called_with(xbounds)
        self.widget.plots.set_plot_x_range.assert_called_with(names,xbounds)

    def test_xRangeChanged1(self):
        names = ["two"]
        xbounds = [9,18]
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =names)
        self.widget._context.set_xBounds = mock.MagicMock()
        self.widget.plots.set_plot_x_range = mock.MagicMock()
        self.widget._x_range_changed(xbounds)
        self.assertEqual(self.widget._context.set_xBounds.call_count, 0)
        self.widget.plots.set_plot_x_range.assert_called_with(names,xbounds)

    def test_yRangeChanged(self):
        names = ["two","three"]
        ybounds = [9,18]
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =names)
        self.widget._context.set_yBounds = mock.MagicMock()
        self.widget.plots.set_plot_y_range = mock.MagicMock()
        self.widget._y_range_changed(ybounds)
        self.widget._context.set_yBounds.assert_called_with(ybounds)
        self.widget.plots.set_plot_y_range.assert_called_with(names,ybounds)

    def test_yRangeChanged1(self):
        names = ["two"]
        ybounds = [9,18]
        # mocks as we only want to test logic
        self.widget.quickEdit.get_selection = mock.MagicMock(return_value =names)
        self.widget._context.set_yBounds = mock.MagicMock()
        self.widget.plots.set_plot_y_range = mock.MagicMock()
        self.widget._y_range_changed(ybounds)
        self.assertEqual(self.widget._context.set_yBounds.call_count, 0)
        self.widget.plots.set_plot_y_range.assert_called_with(names,ybounds)

    def test_checkAllErrorsFalse(self):
        context = data()
        self.widget._context.subplots = context
        self.assertEqual(self.widget._check_all_errors(context.keys()),False)

    def test_checkAllErrorsTrue(self):
        context = data()
        for name in context.keys():
            context[name].error = True
        self.widget._context.subplots = context
        self.assertEqual(self.widget._check_all_errors(context.keys()),True)

    def test_checkAllErrors1True(self):
        context = data()
        context["two"].error = True
        self.widget._context.subplots = context
        self.assertEqual(self.widget._check_all_errors(context.keys()),False)


if __name__ == "__main__":
    unittest.main()
