import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_presenter import EAFittingTabPresenter
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_model import EAFittingTabModel
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_view import EAFittingTabView
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


class EAFittingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = mock.Mock(spec=EAFittingTabModel)
        self.model.context = self.context
        self.model.fitting_context = self.fitting_context
        self.view = mock.Mock(spec=EAFittingTabView)
        self.presenter = EAFittingTabPresenter(self.view, self.model)

    def test_handle_fit_clicked_when_there_are_no_datasets(self):
        self.model.number_of_datasets = 0
        self.presenter.handle_fit_clicked()

        self.view.warning_popup.assert_called_once_with("No data selected for fitting.")

    def test_handle_fit_clicked_when_datasets_are_present(self):
        self.model.number_of_datasets = 3
        self.presenter._perform_fit = mock.Mock()
        self.presenter.handle_fit_clicked()

        self.view.warning_popup.assert_not_called()
        self.model._get_equivalent_binned_or_unbinned_workspaces.assert_called_once()
        self.model.save_current_fit_function_to_undo_data.assert_called_once()
        self.presenter._perform_fit.assert_called_once()


if __name__ == '__main__':
    unittest.main()
