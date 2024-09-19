# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from SANS.sans.common.enums import SANSFacility
from mantidqtinterfaces.sans_isis.gui_logic.models.diagnostics_model import DiagnosticsModel
from mantidqtinterfaces.sans_isis.gui_logic.models.state_gui_model import StateGuiModel
from SANS.sans.test_helper.user_file_test_helper import sample_user_file, create_user_file
from SANS.sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class DiagnosticsPageModelTest(unittest.TestCase):
    def test_that_create_state_creates_correct_state(self):
        user_file_path = create_user_file(sample_user_file)
        user_file_reader = UserFileReaderAdapter(user_file_name=user_file_path, file_information=None)
        user_file_items = user_file_reader.get_all_states(file_information=None)

        state_from_view = StateGuiModel(user_file_items)

        state = DiagnosticsModel.create_state(state_from_view, "SANS2D00022024", "", SANSFacility.ISIS)

        self.assertEqual(state.data.sample_scatter, "SANS2D00022024")


if __name__ == "__main__":
    unittest.main()
