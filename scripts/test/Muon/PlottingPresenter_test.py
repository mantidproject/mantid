import unittest

import os
os.environ["QT_API"] = "pyqt"  # noqa E402

from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.select_subplot import SelectSubplot
from Muon.GUI.ElementalAnalysis.Plotting.plotting_presenter import PlotPresenter
from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView
from Muon.GUI.ElementalAnalysis.Plotting.edit_windows.remove_plot_window import RemovePlotWindowView
from  Muon.GUI.Common import mock_widget

from qtpy import QtCore

try:
    from unittest import mock
except ImportError:
    import mock

# simple class to mock QDialogs
class dummy_popup(object):
    def __init__(self,lines,parent=None):
        self.dummy = True

    def setMinimumSize(self, min1,min2):
        return 

    def show(self):
        return

    def raise_(self):
        return

class PlottingPresenterTest(unittest.TestCase):
    def setUp(self):
        view = mock.create_autospec(PlotView)
        view.subplotRemovedSignal = mock.Mock()
        self.presenter = PlotPresenter(view)
        self.presenter.view.canvas = mock.Mock()
        self.presenter.view.canvas.draw = mock.Mock()

        self.view = self.presenter.view
        self.view.close = mock.Mock()
        self.view.get_subplots = mock.Mock(return_value = {})
        self.view.removeLine = mock.Mock()


        # explicitly mock pop up
        self.mock_selector =  mock.create_autospec(dummy_popup)
        self.mock_selector.subplotSelectorSignal = mock.Mock()
        self.mock_selector.closeEventSignal = mock.Mock()

        self.mock_rmWindow =  mock.create_autospec(dummy_popup)
        self.mock_rmWindow.applyRemoveSignal = mock.Mock()
        self.mock_rmWindow.closeEventSignal = mock.Mock()
        self.mock_rmWindow.subplot = mock.Mock(return_value="plot")
        self.mock_rmWindow.getState = mock.Mock(side_effect = [True, True])
        self.mock_rmWindow.getLine = mock.Mock()

        # mock presenter to create mock pop ups
        self.presenter.createRmWindow = mock.Mock(return_value = self.mock_rmWindow)
        self.presenter.createSelectWindow = mock.Mock(return_value =self.mock_selector)

        self.mock_name = mock.Mock()
        self.mock_workspace = mock.Mock()
        self.mock_func = mock.Mock()
        self.mock_arbitrary_args = [mock.Mock() for i in range(3)]


    def test_setup(self):
        assert(self.view.setRmConnection.call_count ==1)
        assert(self.view.setAddConnection.call_count ==1)
        assert(self.view.plotCloseConnection.call_count ==1)
        self.view.setRmConnection.assert_called_with(self.presenter.rm)
        self.view.setAddConnection.assert_called_with(self.presenter.add)
        self.view.plotCloseConnection.assert_called_with(self.presenter.close)

    def test_get_subplot(self):
        self.presenter.get_subplot(self.mock_name)
        self.view.get_subplot.assert_called_with(self.mock_name)

    def test_get_subplots(self):
        self.presenter.get_subplots()
        self.assertEquals(self.view.get_subplots.call_count, 1)

    def test_add_subplot(self):
        self.presenter.add_subplot(self.mock_name)
        self.view.add_subplot.assert_called_with(self.mock_name)

    def test_plot(self):
        self.presenter.plot(self.mock_name, self.mock_workspace)
        self.view.plot.assert_called_with(self.mock_name, self.mock_workspace)

    def test_remove_subplot(self):
        self.presenter.remove_subplot(self.mock_name)
        self.view.remove_subplot.assert_called_with(self.mock_name)

    def test_update_canvas(self):
        self.presenter.update_canvas()
        self.assertEquals(self.view.canvas.draw.call_count, 1)

    def test_add_moveable_vline(self):
        """
        To be added when moveable vlines are implemented.
        """
        pass

    def test_add_moveable_hline(self):
        """
        To be added when moveable hlines are implemented.
        """
        pass


    def test_removeSubplotConnection(self):
        self.presenter.removeSubplotConnection(self.mock_func)
        assert(self.view.subplotRemovedSignal.connect.call_count ==1)
        self.view.subplotRemovedSignal.connect.assert_called_with(self.mock_func)
   

    def test_closeSelector(self):
        # mock called function to test they are called
        self.presenter.closeSelectorWindow = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()
        # set windows to test them
        self.presenter.selectorWindow = mock.Mock()
        self.presenter.rmWindow = None
        # do test
        self.presenter.close()
        assert(self.presenter.closeSelectorWindow.call_count == 1)
        assert(self.presenter.closeRmWindow.call_count == 0)


    def test_closeRm(self):
        # mock called function to test they are called
        self.presenter.closeSelectorWindow = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()
        # set windows to test them
        self.presenter.selectorWindow = None
        self.presenter.rmWindow = mock.Mock()
        # do test
        self.presenter.close()
        assert(self.presenter.closeSelectorWindow.call_count == 0)
        assert(self.presenter.closeRmWindow.call_count == 1)

    def test_closeNone(self):
        # mock called function to test they are called
        self.presenter.closeSelectorWindow = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()
        # set windows to test them
        self.presenter.selectorWindow = None
        self.presenter.rmWindow = None
        # do test
        self.presenter.close()
        assert(self.presenter.closeSelectorWindow.call_count == 0)
        assert(self.presenter.closeRmWindow.call_count == 0)

    def test_closeAll(self):
        # mock called function to test they are called
        self.presenter.closeSelectorWindow = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()
        # set windows to test them
        self.presenter.selectorWindow = mock.Mock()
        self.presenter.rmWindow = mock.Mock()
        # do test
        self.presenter.close()
        assert(self.presenter.closeSelectorWindow.call_count == 1)
        assert(self.presenter.closeRmWindow.call_count == 1)

    def test_add(self):
        # to do
        pass

    def test_rmOneSubplot(self):
        self.view.subplot_names =  ["one plot"]
        self.presenter.rmWindow = None
        self.presenter.selectorWindow = None
        self.presenter.getRmWindow = mock.Mock()
        # run test
        self.presenter.rm()

        # skip to rm window if subplot == 1
        assert(self.presenter.getRmWindow.call_count == 1)

        # never create selector window
        assert(self.presenter.createSelectWindow.call_count == 0)
        assert(self.mock_selector.subplotSelectorSignal.connect.call_count == 0)
        assert(self.mock_selector.closeEventSignal.connect.call_count == 0)
        assert(self.mock_selector.setMinimumSize.call_count == 0)
        assert(self.mock_selector.show.call_count == 0)
        # never make the remove window or raise any windows
        assert(self.mock_rmWindow.raise_.call_count == 0)
        assert(self.mock_selector.raise_.call_count == 0)
 


       
    def test_rmTwoSubplots(self):
        self.view.subplot_names =  ["one plot","two plots"]
        self.presenter.rmWindow = None
        self.presenter.selectorWindow = None
        self.presenter.getRmWindow = mock.Mock()

        # do test
        self.presenter.rm()
        # create selector window if > 2
        assert(self.presenter.createSelectWindow.call_count == 1)
        self.presenter.createSelectWindow.assert_called_with(self.view.subplot_names)
        assert(self.mock_selector.subplotSelectorSignal.connect.call_count == 1)
        self.mock_selector.subplotSelectorSignal.connect.assert_called_with(self.presenter.getRmWindow)
        assert(self.mock_selector.closeEventSignal.connect.call_count == 1)
        self.mock_selector.closeEventSignal.connect.assert_called_with(self.presenter.closeSelectorWindow)
        assert(self.mock_selector.setMinimumSize.call_count == 1)
        assert(self.mock_selector.show.call_count == 1)
        # never make the remove window or raise any windows
        assert(self.presenter.getRmWindow.call_count == 0)
        assert(self.mock_rmWindow.raise_.call_count == 0)
        assert(self.mock_selector.raise_.call_count == 0)
       
    def test_RaiseRmWindow(self):
        self.view.subplot_names =  ["one plot","two plots"]
        self.presenter.rmWindow =  self.mock_rmWindow
        self.presenter.selectorWindow = None
        self.presenter.getRmWindow = mock.Mock()

        # do test
        self.presenter.rm()
        # raise rm window
        assert(self.mock_rmWindow.raise_.call_count == 1)
        # never create selector
        assert(self.presenter.createSelectWindow.call_count == 0)
        assert(self.mock_selector.subplotSelectorSignal.connect.call_count == 0)
        assert(self.mock_selector.closeEventSignal.connect.call_count == 0)
        assert(self.mock_selector.setMinimumSize.call_count == 0)
        assert(self.mock_selector.show.call_count == 0)
        # never make the remove window or raise selector windows
        assert(self.presenter.getRmWindow.call_count == 0)
        assert(self.mock_selector.raise_.call_count == 0)

    def test_RaiseSelector(self):
        self.view.subplot_names =  ["one plot","two plots"]
        self.presenter.rmWindow =  None
        self.presenter.selectorWindow = self.mock_selector
        self.presenter.getRmWindow = mock.Mock()

        # do test
        self.presenter.rm()
        # raise selector window
        assert(self.mock_selector.raise_.call_count == 1)
        # never create selector
        assert(self.presenter.createSelectWindow.call_count == 0)
        assert(self.mock_selector.subplotSelectorSignal.connect.call_count == 0)
        assert(self.mock_selector.closeEventSignal.connect.call_count == 0)
        assert(self.mock_selector.setMinimumSize.call_count == 0)
        assert(self.mock_selector.show.call_count == 0)
        # never make the remove window or raise selector windows
        assert(self.mock_rmWindow.raise_.call_count == 0)
        assert(self.presenter.getRmWindow.call_count == 0)
    

    def test_getRmWindow(self):

        result = "plot"
        self.presenter.closeSelectorWindow = mock.Mock()
        self.presenter.createRmWindow = mock.Mock(return_value = self.mock_rmWindow)
        # run test
        self.presenter.getRmWindow(result)

        assert(self.presenter.createRmWindow.call_count == 1)
        self.presenter.createRmWindow.assert_called_with(subplot=result)
        assert(self.mock_rmWindow.applyRemoveSignal.connect.call_count == 1)
        self.mock_rmWindow.applyRemoveSignal.connect.assert_called_with(self.presenter.applyRm)
        assert(self.mock_rmWindow.closeEventSignal.connect.call_count == 1)
        self.mock_rmWindow.closeEventSignal.connect.assert_called_with(self.presenter.closeRmWindow)
        assert(self.mock_rmWindow.show.call_count == 1)
        assert(self.mock_rmWindow.setMinimumSize.call_count == 1)
 

    def test_applyRm0(self):
        names = ["line 1","line 2"]
        self.presenter.rmWindow = self.mock_rmWindow
        self.mock_rmWindow.subplot = mock.Mock(return_value="plot")
        self.mock_rmWindow.getState = mock.Mock(side_effect = [False, False])
        self.view.get_subplots = mock.Mock(return_value = {"plot":1})
        self.presenter.remove_subplot = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()

        # do test
        self.presenter.applyRm(names)
        # check both lines
        assert(self.mock_rmWindow.getState.call_count == 2)
        # only remove 1
        assert(self.mock_rmWindow.getLine.call_count == 0)
        assert(self.view.removeLine.call_count == 0)
        # delete subplot
        assert(self.presenter.remove_subplot.call_count == 0)
        # close window but keep plot
        assert(self.presenter.closeRmWindow.call_count ==1)
        assert(self.view.close.call_count == 0)


    def test_applyRm1(self):
        names = ["line 1","line 2"]
        self.presenter.rmWindow = self.mock_rmWindow
        self.mock_rmWindow.getState = mock.Mock(side_effect = [True, False])
        self.presenter.remove_subplot = mock.Mock()
        self.view.get_subplots = mock.Mock(return_value = {"plot":1})
        self.presenter.closeRmWindow = mock.Mock()

        # do test
        self.presenter.applyRm(names)
        # check both lines
        assert(self.mock_rmWindow.getState.call_count == 2)
        # only remove 1
        assert(self.mock_rmWindow.getLine.call_count == 1)
        assert(self.view.removeLine.call_count == 1)
        # keep subplot
        assert(self.presenter.remove_subplot.call_count == 0)
        # close window
        assert(self.presenter.closeRmWindow.call_count ==1)
        assert(self.view.close.call_count == 0)


    def test_applyRm2(self):
        names = ["line 1","line 2"]
        self.presenter.rmWindow = self.mock_rmWindow
        self.presenter.remove_subplot = mock.Mock()
        self.mock_rmWindow.getState = mock.Mock(side_effect = [True, True])
        self.view.get_subplots = mock.Mock(return_value = {"plot":1})
        self.presenter.closeRmWindow = mock.Mock()

        # do test
        self.presenter.applyRm(names)
        # check both lines
        assert(self.mock_rmWindow.getState.call_count == 2)
        # only remove 1
        assert(self.mock_rmWindow.getLine.call_count == 2)
        assert(self.view.removeLine.call_count == 2)
        # delete subplot
        assert(self.presenter.remove_subplot.call_count == 1)
        # close window but keep plot
        assert(self.presenter.closeRmWindow.call_count ==1)
        assert(self.view.close.call_count == 0)


    def test_applyRmAndClose(self):
        names = ["line 1","line 2"]
        self.presenter.rmWindow = self.mock_rmWindow
        self.presenter.remove_subplot = mock.Mock()
        self.presenter.closeRmWindow = mock.Mock()

        # do test
        self.presenter.applyRm(names)
        # check both lines
        assert(self.mock_rmWindow.getState.call_count == 2)
        # only remove 1
        assert(self.mock_rmWindow.getLine.call_count == 2)
        assert(self.view.removeLine.call_count == 2)
        # delete subplot
        assert(self.presenter.remove_subplot.call_count == 1)
        # close window but keep plot
        assert(self.presenter.closeRmWindow.call_count ==1)
        assert(self.view.close.call_count == 1)

if __name__ == "__main__":
    unittest.main()
