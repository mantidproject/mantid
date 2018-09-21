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


class dummy_selector(object):
    subplotSelectorSignal = QtCore.Signal(object)
    closeEventSignal = QtCore.Signal()


    def __init__(self,lines,parent=None):
        self.dummy = True

    def setMinimumSize(self, min1,min2):
        return 

    def show(self):
        return self._show


class PlottingPresenterTest(unittest.TestCase):
    def setUp(self):
        view = mock.create_autospec(PlotView)
        view.subplotRemovedSignal = mock.Mock()
        self.presenter = PlotPresenter(view)
        self.presenter.view.canvas = mock.Mock()
        self.presenter.view.canvas.draw = mock.Mock()
        self.view = self.presenter.view

        self.mock_name = mock.Mock()
        self.mock_workspace = mock.Mock()
        self.mock_func = mock.Mock()
        self.mock_arbitrary_args = [mock.Mock() for i in range(3)]

        RemovePlotWindow = mock.Mock()

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

    @mock.patch("Muon.GUI.ElementalAnalysis.Plotting.edit_windows.select_subplot.SelectSubplot")
    def test_rmOneSubplot(self,mock_selector):
        self.view.subplot_names =  ["one plot"]
        self.presenter.createRmWindow = mock.Mock()
        self.presenter.rmWindow_ = None
        self.presenter.raiseRmWindow = mock.Mock()
        self.presenter.selectorWindow = None
        self.presenter.raiseSelectorWindow = mock.Mock()

        # do test
        self.presenter.rm()
        # just skip to rm window if one subplot
        assert(self.presenter.createRmWindow.call_count == 1)
        self.presenter.createRmWindow.assert_called_with(self.view.subplot_names[0])
        # never make the selector or raise any windows
        assert(mock_selector.call_count == 0)
        assert(self.presenter.raiseRmWindow.call_count == 0)
        assert(self.presenter.raiseSelectorWindow.call_count == 0)
       
    def test_rmTwoSubplots(self):
        self.view.subplot_names =  ["one plot","two plots"]
        self.presenter.createRmWindow = mock.Mock()
        self.presenter.rmWindow_ = None
        self.presenter.raiseRmWindow = mock.Mock()
        self.presenter.selectorWindow = None
        self.presenter.raiseSelectorWindow = mock.Mock()

        selector =  mock.create_autospec(dummy_selector)
        selector.subplotSelectorSignal = mock.Mock()
        selector.closeEventSignal = mock.Mock()
        self.presenter.createSelectWindow = mock.Mock(return_value =selector)

        # do test
        self.presenter.rm()
        # just skip to rm window if one subplot
        assert(self.presenter.createSelectWindow.call_count == 1)
        self.presenter.createSelectWindow.assert_called_with(self.view.subplot_names)
    
        assert(selector.subplotSelectorSignal.connect.call_count == 1)
        selector.subplotSelectorSignal.connect.assert_called_with(self.presenter.createRmWindow)
 
        assert(selector.closeEventSignal.connect.call_count == 1)
        selector.closeEventSignal.connect.assert_called_with(self.presenter.closeSelectorWindow)
        
        assert(selector.setMinimumSize.call_count == 1)
        assert(selector.show.call_count == 1)
        # never make the remove window or raise any windows
        assert(self.presenter.createRmWindow.call_count == 0)
        assert(self.presenter.raiseRmWindow.call_count == 0)
        assert(self.presenter.raiseSelectorWindow.call_count == 0)
    
  

if __name__ == "__main__":
    unittest.main()
