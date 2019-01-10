from mock import Mock


class MockAx:
    def __init__(self):
        self.plot = Mock()
        self.scatter = Mock()
        self.errorbar = Mock()
        self.legend = Mock()
        self.set_xlabel = Mock()
        self.set_ylabel = Mock()


class MockCanvas:
    def __init__(self):
        self.set_window_title = Mock()


class MockFig:
    def __init__(self):
        self.show = Mock()
        self.mock_canvas = MockCanvas()
        self.canvas = Mock(return_value=self.mock_canvas)


class MockPlotLib:
    def __init__(self):
        self.mock_ax = MockAx()
        self.mock_fig = MockFig()
        self.subplots = Mock(return_value=[self.mock_fig, self.mock_ax])
