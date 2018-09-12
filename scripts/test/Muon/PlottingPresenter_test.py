import unittest

from Muon.GUI.ElementalAnalysis.Plotting.plotting_presenter import PlotPresenter
from Muon.GUI.ElementalAnalysis.Plotting.plotting_view import PlotView


try:
    from unittest import mock
except ImportError:
    import mock


class PlottingPresenterTest(unittest.TestCase):
    def setUp(self):
        view = mock.create_autospec(PlotView)
        self.presenter = PlotPresenter(view)
        self.presenter.view.canvas = mock.Mock()
        self.presenter.view.canvas.draw = mock.Mock()
        self.view = self.presenter.view

        self.mock_name = mock.Mock()
        self.mock_workspace = mock.Mock()
        self.mock_func = mock.Mock()
        self.mock_arbitrary_args = [mock.Mock() for i in range(3)]

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


if __name__ == "__main__":
    unittest.main()
