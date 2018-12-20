from __future__ import (absolute_import, division, print_function)
import unittest
from sans.common.enums import SANSFacility
from sans.gui_logic.models.diagnostics_page_model import create_state
from sans.test_helper.user_file_test_helper import sample_user_file, create_user_file
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.user_file.user_file_reader import UserFileReader


class DiagnosticsPageModelTest(unittest.TestCase):
    def setUp(self):
        pass

    def test_that_create_state_creates_correct_state(self):
        user_file_path = create_user_file(sample_user_file)
        user_file_reader = UserFileReader(user_file_path)
        user_file_items = user_file_reader.read_user_file()

        state_from_view =  StateGuiModel(user_file_items)

        state = create_state(state_from_view, "SANS2D00022024", '', SANSFacility.ISIS)

        self.assertEqual(state.data.sample_scatter, "SANS2D00022024")


if __name__ == '__main__':
    unittest.main()
