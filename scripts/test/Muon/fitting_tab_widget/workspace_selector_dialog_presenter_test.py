from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
import unittest
from mantid.py3compat import mock
from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.contexts.context_setup import setup_context

example_data_service_output = ['EMU62260',
                               'EMU62260 Groups',
                               'EMU62260 Pairs',
                               'EMU62260 Raw Data',
                               'EMU62260; Group; bwd; Asymmetry; #1',
                               'EMU62260; Group; bwd; Counts; #1',
                               'EMU62260; Group; fwd; Asymmetry; #1',
                               'EMU62260; Group; fwd; Counts; #1',
                               'EMU62260; Pair Asym; long; #1',
                               'EMU62260_raw_data',
                               'Muon Data',
                               'MUSR62260',
                               'MUSR62260 Groups',
                               'MUSR62260 Pairs',
                               'MUSR62260 Raw Data',
                               'MUSR62260-62262',
                               'MUSR62260-62262 Groups',
                               'MUSR62260-62262 Pairs',
                               'MUSR62260-62262 Raw Data',
                               'MUSR62260-62262; Group; bkwd; Asymmetry; #1',
                               'MUSR62260-62262; Group; bkwd; Counts; #1',
                               'MUSR62260-62262; Group; bottom; Asymmetry; #1',
                               'MUSR62260-62262; Group; bottom; Counts; #1',
                               'MUSR62260-62262; Group; fwd; Asymmetry; #1',
                               'MUSR62260-62262; Group; fwd; Counts; #1',
                               'MUSR62260-62262; Group; top; Asymmetry; #1',
                               'MUSR62260-62262; Group; top; Counts; #1',
                               'MUSR62260-62262; Pair Asym; long; #1',
                               'MUSR62260-62262_raw_data',
                               'MUSR62260; Group; bkwd; Asymmetry; #1',
                               'MUSR62260; Group; bkwd; Counts; #1',
                               'MUSR62260; Group; bottom; Asymmetry; #1',
                               'MUSR62260; Group; bottom; Counts; #1',
                               'MUSR62260; Group; fwd; Asymmetry; #1',
                               'MUSR62260; Group; fwd; Counts; #1',
                               'MUSR62260; Group; top; Asymmetry; #1',
                               'MUSR62260; Group; top; Counts; #1',
                               'MUSR62260; Pair Asym; long; #1',
                               'MUSR62260_raw_data',
                               'MUSR62261',
                               'MUSR62261 Groups',
                               'MUSR62261 Pairs',
                               'MUSR62261 Raw Data',
                               'MUSR62261; Group; bkwd; Asymmetry; #1',
                               'MUSR62261; Group; bkwd; Counts; #1',
                               'MUSR62261; Group; bottom; Asymmetry; #1',
                               'MUSR62261; Group; bottom; Counts; #1',
                               'MUSR62261; Group; fwd; Asymmetry; #1',
                               'MUSR62261; Group; fwd; Counts; #1',
                               'MUSR62261; Group; top; Asymmetry; #1',
                               'MUSR62261; Group; top; Counts; #1',
                               'MUSR62261; Pair Asym; long; #1',
                               'MUSR62261_raw_data',
                               'MUSR62262',
                               'MUSR62262 Groups',
                               'MUSR62262 Pairs',
                               'MUSR62262 Raw Data',
                               'MUSR62262; Group; bkwd; Asymmetry; #1',
                               'MUSR62262; Group; bkwd; Counts; #1',
                               'MUSR62262; Group; bottom; Asymmetry; #1',
                               'MUSR62262; Group; bottom; Counts; #1',
                               'MUSR62262; Group; fwd; Asymmetry; #1',
                               'MUSR62262; Group; fwd; Counts; #1',
                               'MUSR62262; Group; top; Asymmetry; #1',
                               'MUSR62262; Group; top; Counts; #1',
                               'MUSR62262; Pair Asym; long; #1',
                               'MUSR62262_raw_data',
                               'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                               'MUSR62260; PhaseQuad; PhaseTable MUSR62261']


class WorkspaceSelectorPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.current_runs = [[22725]]
        self.context = setup_context()
        self.context.get_names_of_workspaces_to_fit = mock.MagicMock(return_value=['MUSR22725; Group; fwd; Asymmetry; #1'])
        self.view = WorkspaceSelectorView(self.current_runs, 'MUSR', [], self.context)
        self.view.list_selector_presenter = mock.MagicMock()
        self.context.get_names_of_workspaces_to_fit.reset_mock()

    def test_handle_group_pair_selection_changed(self):
        self.view.group_pair_line_edit.setText('fwd, bwd')
        self.view.group_pair_line_edit.editingFinished.emit()

        self.context.get_names_of_workspaces_to_fit.assert_called_once_with(group_and_pair='fwd, bwd', phasequad=False,
                                                                            rebin=False, runs=[[22725]])

        self.view.list_selector_presenter.update_model.assert_called_once_with({'MUSR22725; Group; fwd; Asymmetry; #1': [0, False, True]})


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
