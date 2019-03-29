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

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.observer_pattern import Observer
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FrequencyContext

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


class MuonDataContextTest(unittest.TestCase):
    def setUp(self):
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext()
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)
        self.frequency_context = FrequencyContext(self.context)
        self.gui_variable_observer = Observer()
        self.gui_variable_observer.update = mock.MagicMock()
        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.data_context.instrument = 'CHRONUS'
        self.gui_variable_observer = Observer()
        self.gui_variable_observer.update = mock.MagicMock()
        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)

        filepath = FileFinder.findRuns('CHRONUS00003422.nxs')[0]

        load_result, run, filename = load_workspace_from_filename(filepath)

        self.loaded_data.add_data(workspace=load_result, run=[run], filename=filename, instrument='CHRONUS')
        self.data_context.current_runs = [[run]]
        self.context.update_current_data()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_get_detectors_excluded_from_default_grouping_tables_gets_correct_groups_for_CHRONUS(self):
        result = self.frequency_context.get_detectors_excluded_from_default_grouping_tables()

        self.assertEqual(result, [256, 425])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)