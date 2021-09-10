# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from Muon.GUI.Common.load_file_widget.model import BrowseFileWidgetModel
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


@start_qapplication
class LoadFileWidgetModelTest(unittest.TestCase):

    def setUp(self):
        setup_context_for_tests(self)
        self.data_context.instrument = 'EMU'
        self.model = BrowseFileWidgetModel(self.loaded_data, self.context)

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

        # We make the filename all uppercase to deal with some os's capitalising the instrument and some not.
        # We just need to check the right files were added to the model.
        expected_files = ['EMU00019489.NXS', 'EMU00006473.NXS', 'EMU00006475.NXS']
        self.assertCountEqual([os.path.split(filename.upper())[-1] for filename in self.model.loaded_filenames], expected_files)
        self.assertCountEqual(self.model.loaded_runs, [[19489], [6473], [6475]])

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

        # We make the filename all uppercase to deal with some os's capitalising the instrument and some not.
        # We just need to check the right files were added to the model.
        self.assertCountEqual([os.path.split(filename.upper())[-1] for filename in self.model.loaded_filenames],
                              ['EMU00019489.NXS', 'EMU00006473.NXS'])
        self.assertCountEqual(self.model.loaded_runs, [[19489], [6473]])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
