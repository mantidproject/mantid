# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest

from mantid.api import AnalysisDataService
from mantid.api import FileFinder
from mantid.dataobjects import Workspace2D

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename

if sys.version_info.major < 2:
    pass
else:
    pass

class MuonContextTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pass

    def setUp(self):
        AnalysisDataService.clear()
        self.filepath = FileFinder.findRuns('EMU00019489.nxs')[0]
        self.load_result, self.run_number, self.filename = load_workspace_from_filename(self.filepath)
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_pair_context = MuonGroupPairContext()
        self.gui_context.update({'RebinType': 'None'})

        self.context = MuonContext(muon_data_context=self.data_context, muon_gui_context=self.gui_context, muon_group_context=self.group_pair_context)

        self.data_context.instrument = 'EMU'

        self.loaded_data.add_data(workspace=self.load_result, run=[self.run_number], filename=self.filename,
                                  instrument='EMU')
        self.data_context.current_runs = [[self.run_number]]
        self.data_context.update_current_data()
        self.group_pair_context.reset_group_and_pairs_to_default(self.load_result['OutputWorkspace'][0]._workspace,
                                                                 'EMU', '')

    def test_reset_groups_and_pairs_to_default(self):
        self.assertEquals(self.group_pair_context.group_names, ['fwd', 'bwd'])
        self.assertEquals(self.group_pair_context.pair_names, ['long'])

    def test_calculate_group_calculates_group_for_given_run(self):
        counts_workspace, asymmetry_workspace = self.context.calculate_group('fwd', run=[19489])

        self.assertEquals(type(counts_workspace), Workspace2D)
        self.assertEquals(type(counts_workspace), Workspace2D)

    def test_calculate_pair_calculates_pair_for_given_run(self):
        pair_asymmetry = self.context.calculate_pair('long', run=[19489])

        self.assertEquals(type(pair_asymmetry), Workspace2D)

    def test_show_all_groups_calculates_and_shows_all_groups(self):
        self.context.show_all_groups()

        self.assertEquals(AnalysisDataService.getObjectNames(), ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1',
                                                                 'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; fwd; Asymmetry; #1',
                                                                 'EMU19489; Group; fwd; Counts; #1', 'Muon Data'])

    def test_that_show_all_calculates_and_shows_all_groups_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_groups()

        self.assertEquals(AnalysisDataService.getObjectNames(),
                          ['EMU19489', 'EMU19489 Groups', 'EMU19489; Group; bwd; Asymmetry; #1', 'EMU19489; Group; bwd; Asymmetry; Rebin; #1',
                           'EMU19489; Group; bwd; Counts; #1', 'EMU19489; Group; bwd; Counts; Rebin; #1',
                           'EMU19489; Group; fwd; Asymmetry; #1', 'EMU19489; Group; fwd; Asymmetry; Rebin; #1',
                           'EMU19489; Group; fwd; Counts; #1', 'EMU19489; Group; fwd; Counts; Rebin; #1', 'Muon Data'])

    def test_show_all_pairs_calculates_and_shows_all_pairs(self):
        self.context.show_all_pairs()

        self.assertEquals(AnalysisDataService.getObjectNames(), ['EMU19489', 'EMU19489 Pairs', 'EMU19489; Pair Asym; long; #1', 'Muon Data'])

    def test_that_show_all_calculates_and_shows_all_pairs_with_rebin(self):
        self.gui_context['RebinType'] = 'Fixed'
        self.gui_context['RebinFixed'] = 2

        self.context.show_all_pairs()

        self.assertEquals(AnalysisDataService.getObjectNames(),
                          ['EMU19489', 'EMU19489 Pairs', 'EMU19489; Pair Asym; long; #1', 'EMU19489; Pair Asym; long; Rebin; #1', 'Muon Data'])

    def test_update_current_data_sets_current_run_in_data_context(self):
        self.context.update_current_data()

        self.assertEquals(self.data_context.current_data, self.load_result)

    def test_update_current_data_sets_groups_and_pairs(self):
        self.context.update_current_data()

        self.assertEquals(self.group_pair_context.pair_names, ['long'])
        self.assertEquals(self.group_pair_context.group_names, ['fwd', 'bwd'])

    def test_show_raw_data_puts_raw_data_into_the_ADS(self):
        self.context.show_raw_data()

        self.assertEquals(AnalysisDataService.getObjectNames(), ['EMU19489', 'EMU19489 Raw Data', 'EMU19489_raw_data', 'Muon Data'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)