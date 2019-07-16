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

from Muon.GUI.Common.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename

if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock
from mantidqt.utils.qt.testing import GuiTest


class MuonDataContextTest(GuiTest):
    def setUp(self):
        setup_context_for_tests(self)
        self.gui_variable_observer = Observer()
        self.gui_variable_observer.update = mock.MagicMock()
        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.data_context.instrument = 'CHRONUS'
        self.gui_variable_observer = Observer()
        self.gui_variable_observer.update = mock.MagicMock()
        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)

        filepath = FileFinder.findRuns('CHRONUS00003422.nxs')[0]

        load_result, run, filename, _ = load_workspace_from_filename(filepath)

        self.loaded_data.add_data(workspace=load_result, run=[run], filename=filename, instrument='CHRONUS')
        self.data_context.current_runs = [[run]]
        self.context.update_current_data()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_get_detectors_excluded_from_default_grouping_tables_gets_correct_groups_for_CHRONUS(self):
        pass


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
