import unittest
from mantid.py3compat import mock
from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_presenter import FittingTabPresenter
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView
from Muon.GUI.Common.contexts.context_setup import setup_context


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class FittingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.view = FittingTabView()
        self.context = setup_context()
        self.context.data_context.current_runs = [[62260]]
        self.context.data_context.instrument = 'MUSR'
        self.presenter = FittingTabPresenter(self.view, self.context)

    @mock.patch('Muon.GUI.Common.fitting_tab_widget.fitting_tab_presenter.WorkspaceSelectorView.get_selected_data')
    def test_handle_select_fit_data_clicked_updates_current_run_list(self, dialog_mock):
        dialog_mock.return_value = (['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                                     'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                                     'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                                     'MUSR62260; PhaseQuad; PhaseTable MUSR62261'], True)

        self.presenter.handle_select_fit_data_clicked()

        dialog_mock.assert_called_once_with([[62260]], 'MUSR', self.view)

        self.assertEqual(retrieve_combobox_info(self.view.parameter_display_combo),
                         ['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                          'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                          'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                          'MUSR62260; PhaseQuad; PhaseTable MUSR62261'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
