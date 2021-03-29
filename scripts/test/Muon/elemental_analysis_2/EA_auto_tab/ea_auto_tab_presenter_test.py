import unittest
import unittest.mock as mock
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_view import EAAutoTabView
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter import EAAutoTabPresenter
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import EAAutoTabModel
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_match_table_presenter import EAMatchTablePresenter


class EAAutoTabPresenterTest(unittest.TestCase):

    def setUp(self):
        self.presenter = EAAutoTabPresenter(None ,mock.Mock() ,mock.Mock() , mock.Mock() )

    def test_run_find_peak_algorithms_if_parameters_is_None(self):
        self.presenter.view.get_parameters_for_find_peaks.return_value = None

        self.presenter.run_find_peak_algorithms()

        self.presenter.model.run_peak_algorithms.assert_not_called()

    def test_run_find_peak_algorithms_if_parameters_is_valid(self):
        self.presenter.view.get_parameters_for_find_peaks.return_value = 3

        self.presenter.run_find_peak_algorithms()

        self.presenter.model.run_peak_algorithms.assert_called_once_with(3)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_table.EAAutoPopupTable")
    def test_show_peaks_table(self , mock_ea_auto_popup_table):
        pass

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_table.EAAutoPopupTable")
    def test_show_match_table(self , mock_ea_auto_popup_table):
        pass


if __name__ == '__main__':
    unittest.main()
