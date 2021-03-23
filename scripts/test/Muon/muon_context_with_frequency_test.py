# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import AnalysisDataService, FileFinder
from unittest import mock
from mantid import ConfigService
from mantid.simpleapi import CreateWorkspace
from collections import Counter
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.test_helpers.context_setup import setup_context


# Ony want to test frequency specific aspects
# the rest is covered in muon_context_test.py
@start_qapplication
class MuonContextWithFrequencyTest(unittest.TestCase):
    def setUp(self):
        AnalysisDataService.clear()
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'True'
        self.filepath = FileFinder.findRuns('EMU00019489.nxs')[0]

        self.load_result, self.run_number, self.filename, psi_data, _ = load_workspace_from_filename(self.filepath)
        self.assert_(not psi_data)

        self.context = setup_context(True)
        self.context.gui_context.update({'RebinType': 'None'})
        self.loaded_data = self.context.data_context._loaded_data
        self.data_context = self.context.data_context
        self.gui_context = self.context.gui_context
        self.group_pair_context = self.context.group_pair_context
        self.data_context.instrument = 'EMU'

        self.loaded_data.add_data(workspace=self.load_result, run=[self.run_number], filename=self.filename,
                                  instrument='EMU')
        self.data_context.current_runs = [[self.run_number]]
        self.data_context.update_current_data()
        self.group_pair_context.reset_group_and_pairs_to_default(self.load_result['OutputWorkspace'][0].workspace,
                                                                 'EMU', '', 1)

    def tearDown(self):
        ConfigService['MantidOptions.InvisibleWorkspaces'] = 'False'

    def populate_ADS(self):
        self.context.calculate_all_groups()
        self.context.show_all_groups()
        self.context.calculate_all_pairs()
        self.context.show_all_pairs()
        CreateWorkspace([0], [0], OutputWorkspace='EMU19489; PhaseQuad; PhaseTable EMU19489')
        self.context.phase_context.add_phase_quad(
            MuonWorkspaceWrapper('EMU19489; PhaseQuad; PhaseTable EMU19489'), '19489')

    def test_window(self):
        self.assertEquals("Frequency Domain Analysis", self.context.window_title)

    def test_get_workspace_names_returns_no_time_domain_workspaces(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit('19489', 'fwd, bwd, long', True)
        self.assertEqual(Counter(workspace_list),
                         Counter())

    def test_get_workspace_names_returns_nothing_if_no_parameters_passed(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit(freq = "All")

        self.assertEqual(workspace_list, [])

    def test_get_workspaces_names_copes_with_no_freq_runs(self):
        self.populate_ADS()
        workspace_list = self.context.get_names_of_workspaces_to_fit(runs='19489', group_and_pair='fwd, bwd, long, random, wrong',
                                                                     phasequad=True, freq="All")

        self.assertEqual(Counter(workspace_list),
                         Counter([]))

    def test_call_freq_workspace_names(self):
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.context.get_names_of_workspaces_to_fit(runs='19489', group_and_pair='fwd, bwd', freq="All")
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with(runs='19489', group_and_pair='fwd, bwd',
                                                                                             frequency_type="All")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
