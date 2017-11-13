import unittest
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.model.add_runs_model import (AddRunsModel, BinningType)
from ui.sans_isis.add_runs_page import AddRunsPage

if sys.version_info.major == 3:
     from unittest import mock
else:
     import mock

class AddRunsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.model = self._make_mock_model()
        self.presenter = self._make_presenter(self.model, self.view)

    def _make_mock_view(self):
        return mock.create_autospec(AddRunsPage, spec_set=True)

    def _make_mock_model(self):
        return mock.create_autospec(AddRunsModel)

    def _make_presenter(self, model, view):
        return AddRunsPagePresenter(model, view)

    def test_add_runs_adds_run_to_model(self):
        run_name = 'This is a run'
        self.model.add_run(run_name)
        self.model.add_run.assert_called_with(run_name)

if __name__ == '__main__': unittest.main()    
