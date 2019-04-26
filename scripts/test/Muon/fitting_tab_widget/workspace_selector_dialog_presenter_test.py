from Muon.GUI.Common.fitting_tab_widget.workspace_selector_presenter import WorkspaceSelectorPresenter
from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
import unittest
from mantid.py3compat import mock
from Muon.GUI.Common import mock_widget

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
        self.view = WorkspaceSelectorView()
        self.current_runs = [[62260]]
        self.presenter = WorkspaceSelectorPresenter(self.current_runs, 'MUSR', self.view)

    @mock.patch('Muon.GUI.Common.fitting_tab_widget.workspace_selector_presenter.AnalysisDataService')
    def test_get_workspace_list_succesfully_filters_for_single_run(self, data_service_mock):
        data_service_mock.getObjectNames.return_value = example_data_service_output

        workspace_list = self.presenter.get_workspace_list()

        self.assertEquals(workspace_list, ['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                                           'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                                           'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                                           'MUSR62260; PhaseQuad; PhaseTable MUSR62261'])

    @mock.patch('Muon.GUI.Common.fitting_tab_widget.workspace_selector_presenter.AnalysisDataService')
    def test_get_workspace_list_works_for_multiple_runs(self, data_service_mock):
        self.presenter.current_runs = [[62260], [62262]]
        data_service_mock.getObjectNames.return_value = example_data_service_output

        workspace_list = self.presenter.get_workspace_list()

        self.assertEquals(sorted(workspace_list),
                          sorted(['MUSR62260; Group; bkwd; Asymmetry; #1', 'MUSR62260; Group; bottom; Asymmetry; #1',
                           'MUSR62260; Group; fwd; Asymmetry; #1', 'MUSR62260; Group; top; Asymmetry; #1',
                           'MUSR62260; Pair Asym; long; #1', 'MUSR62260; PhaseQuad; PhaseTable MUSR62260',
                           'MUSR62260; PhaseQuad; PhaseTable MUSR62261', 'MUSR62262; Group; bkwd; Asymmetry; #1',
                           'MUSR62262; Group; bottom; Asymmetry; #1', 'MUSR62262; Group; fwd; Asymmetry; #1',
                           'MUSR62262; Group; top; Asymmetry; #1', 'MUSR62262; Pair Asym; long; #1']))

    @mock.patch('Muon.GUI.Common.fitting_tab_widget.workspace_selector_presenter.AnalysisDataService')
    def test_get_workspace_list_succesfully_filters_for_co_added_runs(self, data_service_mock):
        data_service_mock.getObjectNames.return_value = example_data_service_output
        self.presenter.current_runs = [[62260, 62261, 62262]]

        workspace_list = self.presenter.get_workspace_list()

        self.assertEquals(workspace_list,
                          ['MUSR62260-62262; Group; bkwd; Asymmetry; #1',
                           'MUSR62260-62262; Group; bottom; Asymmetry; #1',
                           'MUSR62260-62262; Group; fwd; Asymmetry; #1',
                           'MUSR62260-62262; Group; top; Asymmetry; #1',
                           'MUSR62260-62262; Pair Asym; long; #1'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
