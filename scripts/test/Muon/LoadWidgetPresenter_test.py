import unittest

from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel

from Muon.GUI.Common import mock_widget

try:
    from unittest import mock
except ImportError:
    import mock


class LoadPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()

        self._view = mock.create_autospec(LoadView)
        self._load_model = mock.create_autospec(LoadModel)
        self._co_model = mock.create_autospec(CoLoadModel)
        self.presenter = LoadPresenter(
            self._view, self._load_model, self._co_model)

        self.view = self.presenter.view

    # checks if subsequent function is called on func()
    def check_second_func_called(self, func, sub_func):
        func(mock.Mock())
        assert sub_func.call_count == 1


if __name__ == "__main__":
    unittest.main()
