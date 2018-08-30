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

    def test_equalise_loaded_runs(self):
        self.presenter.co_model.loaded_runs = 5
        self.presenter.load_model.loaded_runs = 10
        self.presenter.equalise_loaded_runs()
        self.assertEquals(self.presenter.co_model.loaded_runs, 10)

    def test_update_models(self):
        self.presenter.load_model = LoadModel()
        self.presenter.co_model = CoLoadModel()
        test_value = 10
        self.presenter.update_models(test_value)
        self.assertEquals(self.presenter.load_model.run, test_value)
        self.assertEquals(self.presenter.co_model.run, test_value)

    def test_enable_buttons(self):
        self.presenter.enable_buttons()
        self.assertEquals(self.view.enable_buttons.call_count, 1)

    def test_disable_buttons(self):
        self.presenter.disable_buttons()
        self.assertEquals(self.view.disable_buttons.call_count, 1)


if __name__ == "__main__":
    unittest.main()
