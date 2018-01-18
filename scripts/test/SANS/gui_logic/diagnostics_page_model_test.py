from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from sans.test_helper.mock_objects import create_mock_diagnostics_tab
from sans.common.enums import SANSInstrument, DetectorType, IntegralEnum, SANSFacility
from sans.gui_logic.models.diagnostics_page_model import create_state
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
from sans.test_helper.user_file_test_helper import sample_user_file, create_user_file
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.user_file.user_file_reader import UserFileReader

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

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