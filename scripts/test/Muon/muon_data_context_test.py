# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.muon_data_context import MuonDataContext
from mantid.api import AnalysisDataService
import unittest
from Muon.GUI.Common.observer_pattern import Observer
from mantid.api import FileFinder



if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


class MuonDataContextTest(unittest.TestCase):
    def setUp(self):
        self.loaded_data = MuonLoadData()
        self.context = MuonDataContext(self.loaded_data)
        self.context.instrument = 'EMU'
        self.gui_variable_observer = Observer()
        self.gui_variable_observer.update = mock.MagicMock()
        self.context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)

        filepath = FileFinder.findRuns('EMU00019489.nxs')[0]

        load_result, run, filename = load_workspace_from_filename(filepath)

        self.loaded_data.add_data(workspace=load_result, run=[run], filename=filename, instrument='EMU')
        self.context.current_runs = [[run]]
        self.context.update_current_data()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_that_setting_a_fixed_rebin_step_calculates_all_groups_and_pairs_twice(self):
        self.context.gui_variables['RebinType'] = 'Fixed'
        self.context.gui_variables['RebinFixed'] = '2'

        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1', 'EMU19489; Group; bwd; Asymmetry; Rebin; #1',
                               'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; bwd; Counts; Rebin; #1', 'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Asymmetry; Rebin; #1', 'EMU19489; Group; fwd; Counts; #1', 'EMU19489; Group; fwd; Counts; Rebin; #1',
                               'Muon Data']


        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_that_setting_no_rebinning_calculates_groups_and_pairs_once(self):
        self.context.gui_variables['RebinType'] = 'None'
        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups',
                               'EMU19489; Group; bwd; Asymmetry; #1',
                               'EMU19489; Group; bwd; Counts; #1',
                               'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Counts; #1',
                               'Muon Data']

        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_that_setting_a_variable_rebin_step_calculates_all_groups_and_pairs_twice(self):
        self.context.gui_variables['RebinType'] = 'Variable'
        self.context.gui_variables['RebinVariable'] = '1,0.1,2'

        self.context.show_all_groups()

        expected_workspaces = ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1', 'EMU19489; Group; bwd; Asymmetry; Rebin; #1',
                               'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; bwd; Counts; Rebin; #1', 'EMU19489; Group; fwd; Asymmetry; #1',
                               'EMU19489; Group; fwd; Asymmetry; Rebin; #1', 'EMU19489; Group; fwd; Counts; #1', 'EMU19489; Group; fwd; Counts; Rebin; #1',
                               'Muon Data']


        self.assertEqual(AnalysisDataService.getObjectNames(), expected_workspaces)

    def test_when_gui_variables_is_modified_notifies_observer(self):
        self.context.add_or_replace_gui_variables(RebinType='Fixed')

        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)