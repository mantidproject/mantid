from __future__ import (absolute_import, division, print_function)
import unittest
import sys
from sans.test_helper.mock_objects import create_mock_diagnostics_tab
from sans.common.enums import SANSInstrument, DetectorType, IntegralEnum, SANSFacility
from sans.gui_logic.models.diagnostics_page_model import create_state
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock)
from sans.test_helper.user_file_test_helper import sample_user_file
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.gui_logic.models.state_gui_model import StateGuiModel

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

class DiagnosticsPageModelTest(unittest.TestCase):
    def setUp(self):
        pass

    def test_that_create_state_creates_correct_state(self):
        state_from_view =  StateGuiModel(sample_user_file)
        state = create_state(state_from_view, '34484', 0, SANSFacility.ISIS)

        self.assertEqual(state.data.sample_scatter, '34484')

if __name__ == '__main__':
    unittest.main()