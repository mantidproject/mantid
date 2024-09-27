# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans_core.state.AllStates import AllStates
from sans_core.user_file.txt_parsers.ParsedDictConverter import ParsedDictConverter


class CommandInterfaceAdapter(ParsedDictConverter):
    def __init__(self, file_information, processed_state, existing_state_obj: AllStates = None):
        self._processed_state = processed_state
        super(CommandInterfaceAdapter, self).__init__(file_information=file_information, existing_all_states=existing_state_obj)

    def _get_input_dict(self):
        return self._processed_state
