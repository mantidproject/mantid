from mock import Mock


class MockPlot:
    MOCK_PLOT_COLOR = "#notacolor"

    def __init__(self):
        self.get_color = Mock(return_value=self.MOCK_PLOT_COLOR)


class MockPlotLibAx:
    def __init__(self):
        self.mock_plot = MockPlot()
        self.plot = Mock(return_value=self.mock_plot)
        self.errorbar = Mock()
        self.legend = Mock()


class MockPlotLibFig:
    def __init__(self):
        self.show = Mock()


class MockPlotLib:
    """
    Mocks the Matplotlib interface for testing
    """

    def __init__(self):
        self.mock_fig = MockPlotLibFig()
        self.mock_ax = MockPlotLibAx()
        self.subplots = Mock(return_value=[self.mock_fig, self.mock_ax])
