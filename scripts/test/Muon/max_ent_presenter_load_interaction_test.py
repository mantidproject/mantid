# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.api import FileFinder
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest
from qtpy import QtCore

from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view_new
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.utilities import load_utils
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_model
from Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter_new
from Muon.GUI.Common.test_helpers.general_test_helpers import create_workspace_wrapper_stub_object
from mantid.api import Workspace


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class MaxEntPresenterTest(GuiTest):
    def setUp(self):
        self.context = setup_context(True)

        self.context.data_context.instrument = 'MUSR'

        self.context.gui_context.update({'RebinType': 'None'})
        self.model = maxent_model.MaxEntModel()

        self.view = maxent_view_new.MaxEntView()

        self.presenter = maxent_presenter_new.MaxEntPresenter(self.view, self.context)

        file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
        ws, run, filename, _ = load_utils.load_workspace_from_filename(file_path)
        self.context.data_context._loaded_data.remove_data(run=run)
        self.context.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
        self.context.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
        self.context.group_pair_context.add_pair(pair=test_pair)

        self.view.warning_popup = mock.MagicMock()

    def test_get_workspace_names_sets_comboboxes_appropriately(self):
        self.presenter.getWorkspaceNames()

        self.assertEquals(retrieve_combobox_info(self.view.ws), ['MUSR22725_raw_data MA'])
        self.assertEquals(retrieve_combobox_info(self.view.N_points),
                          ['2048', '4096', '8192', '16384', '32768', '65536',
                           '131072', '262144', '524288', '1048576'])

    def test_get_parameters_for_maxent_calculations(self):
        self.presenter.getWorkspaceNames()
        self.context.dead_time_table = mock.MagicMock(return_value='deadtime_table_name')
        self.context.first_good_data = mock.MagicMock(return_value=0.11)
        self.context.last_good_data = mock.MagicMock(return_value=13.25)
        self.context.phase_context.phase_tables = [create_workspace_wrapper_stub_object(x) for x in
                                                   ['MUSR22222_phase_table', 'MUSR33333_phase_table',
                                                    'EMU22222_phase_table']]
        self.presenter.update_phase_table_options()

        parameters = self.presenter.get_parameters_for_maxent_calculation()

        self.assertEqual(parameters, {'DefaultLevel': 0.1, 'DoublePulse': False, 'Factor': 1.04, 'FirstGoodTime': 0.11,
                                       'FitDeadTime': True, 'InnerIterations': 10,
                                       'InputDeadTimeTable': 'deadtime_table_name',
                                       'InputWorkspace': 'MUSR22725_raw_data MA',
                                       'LastGoodTime': 13.25, 'MaxField': 1000.0, 'Npts': 2048, 'OuterIterations': 10})

    def test_update_phase_table_options_adds_correct_options_to_view_item(self):
        phase_table_names = ['MUSR22222_phase_table', 'MUSR33333_phase_table', 'EMU22222_phase_table']
        self.context.phase_context.phase_tables = [create_workspace_wrapper_stub_object(x) for x in phase_table_names]

        self.presenter.update_phase_table_options()

        self.assertEqual(retrieve_combobox_info(self.view.phase_table_combo),
                          ['Construct', 'MUSR22222_phase_table', 'MUSR33333_phase_table'])

    @mock.patch('Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter_new.MuonWorkspaceWrapper')
    def test_add_maxent_workspace_to_ADS(self, workspace_wrapper_mock):
        self.presenter.getWorkspaceNames()
        self.context.dead_time_table = mock.MagicMock(return_value='deadtime_table_name')
        self.context.first_good_data = mock.MagicMock(return_value=0.11)
        self.context.last_good_data = mock.MagicMock(return_value=13.25)
        self.context.phase_context.phase_tables = [create_workspace_wrapper_stub_object(x) for x in
                                                   ['MUSR22222_phase_table', 'MUSR33333_phase_table',
                                                    'EMU22222_phase_table']]
        self.presenter.update_phase_table_options()
        maxent_workspace = mock.MagicMock(spec=Workspace)

        self.presenter.add_maxent_workspace_to_ADS('MUSR22725_MaxEnt', maxent_workspace, mock.MagicMock())

        workspace_wrapper_mock.assert_called_once_with(maxent_workspace,
                                                       'MUSR22725 Maxent MA/MUSR22725_MaxEnt; MaxEnt')
        workspace_wrapper_mock.return_value.show.assert_called_once_with()

    def test_get_output_options_defaults_returns_correctly(self):
        self.presenter.getWorkspaceNames()

        output_options = self.presenter.get_maxent_output_options()

        self.assertEqual(output_options, {'OutputDeadTimeTable': False, 'PhaseConvergenceTable': False,
                                           'OutputPhaseTable': False, 'ReconstructedSpectra': False})

    def test_get_output_options_returns_correctly(self):
        self.presenter.getWorkspaceNames()
        self.view.output_dead_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_phase_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_phase_evo_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_data_box.setCheckState(QtCore.Qt.Checked)

        output_options = self.presenter.get_maxent_output_options()

        self.assertEqual(output_options, {'OutputDeadTimeTable': True, 'PhaseConvergenceTable': True,
                                           'OutputPhaseTable': True, 'ReconstructedSpectra': True})


if __name__ == '__main__':
    unittest.main()
