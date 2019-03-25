# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys
import unittest
from Muon.GUI.Common.muon_context import MuonContext
from Muon.GUI.Common.observer_pattern import Observer
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
import sys
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_group_pair_context import MuonGroupPairContext
from mantid.api import AnalysisDataService
import unittest
from Muon.GUI.Common.observer_pattern import Observer
from mantid.api import FileFinder

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock

class MuonContextTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.filepath = FileFinder.findRuns('EMU00019489.nxs')[0]
        cls.load_result, cls.run_number, cls.filename = load_workspace_from_filename(cls.filepath)

    def setUp(self):
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_pair_context = MuonGroupPairContext()

        self.context = MuonContext(muon_data_context=self.data_context, muon_gui_context=self.gui_context, muon_group_context=self.group_pair_context)

        self.data_context.instrument = 'EMU'

        self.loaded_data.add_data(workspace=self.load_result, run=[self.run_number], filename=self.filename,
                                  instrument='EMU')
        self.data_context.current_runs = [[self.run_number]]
        self.data_context.update_current_data()

    def test_calculate_group_calculates_group_for_given_run_and_stores_appropriately(self):
        self.context.calculate_group('fwd', run=[19489])



if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)