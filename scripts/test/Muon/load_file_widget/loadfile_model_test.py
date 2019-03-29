import os
import unittest

import six

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.muon_load_data import MuonLoadData


class LoadFileWidgetModelTest(unittest.TestCase):

    def setUp(self):
        self.data = MuonLoadData()
        self.data_context = MuonDataContext(self.data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext()
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)
        self.data_context.instrument = 'EMU'
        self.model = BrowseFileWidgetModel(self.data, self.context)

    def assert_model_empty(self):
        self.assertEqual(self.model.loaded_workspaces, [])
        self.assertEqual(self.model.loaded_filenames, [])
        self.assertEqual(self.model.loaded_runs, [])

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        self.assert_model_empty()

    def test_executing_load_without_filenames_does_nothing(self):
        self.model.execute()
        self.assert_model_empty()

    def test_execute_successfully_loads_given_files(self):
        files = ['EMU00019489.nxs', 'emu00006473.nxs', 'emu00006475.nxs']
        self.model.loadData(files)

        self.model.execute()

        six.assertCountEqual(self, [os.path.split(filename)[-1] for filename in self.model.loaded_filenames], files)
        six.assertCountEqual(self, self.model.loaded_runs, [[19489], [6473], [6475]])

    def test_model_is_cleared_correctly(self):
        files = ['EMU00019489.nxs', 'emu00006473.nxs', 'emu00006475.nxs']
        self.model.loadData(files)

        self.model.execute()
        self.assertEqual(len(self.model.loaded_filenames), 3)
        self.model.clear()

        self.assert_model_empty()

    def test_execute_throws_if_one_file_does_not_load_correctly_but_still_loads_other_files(self):
        files = ['EMU00019489.nxs', 'emu00006473.nxs', 'NonExistent.nxs']

        self.model.loadData(files)
        with self.assertRaises(ValueError):
            self.model.execute()

        six.assertCountEqual(self, [os.path.split(filename)[-1] for filename in self.model.loaded_filenames],
                             ['EMU00019489.nxs', 'emu00006473.nxs'])
        six.assertCountEqual(self, self.model.loaded_runs, [[19489], [6473]])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
