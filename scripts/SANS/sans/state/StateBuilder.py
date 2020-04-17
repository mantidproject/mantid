# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from sans.state.IStateParser import IStateParser
from sans.state.StateRunDataBuilder import StateRunDataBuilder
from sans.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class StateBuilder(IStateParser):
    @staticmethod
    def new_instance(file_information, data_info, user_filename):
        run_data = StateRunDataBuilder(file_information=file_information)
        user_file = UserFileReaderAdapter(data_info=data_info, user_file_name=user_filename)
        return StateBuilder(run_data_builder=run_data, i_state_parser=user_file)

    def __init__(self, i_state_parser, run_data_builder):
        self._file_parser = i_state_parser
        self._run_data_builder = run_data_builder

    def get_all_states(self):
        state = self._file_parser.get_all_states()
        return self._run_data_builder.pack_all_states(state)

    def get_state_adjustment(self):
        return self._file_parser.get_state_adjustment()

    def get_state_calculate_transmission(self):
        return self._file_parser.get_state_calculate_transmission()

    def get_state_compatibility(self):
        return self._file_parser.get_state_compatibility()

    def get_state_convert_to_q(self):
        return self._file_parser.get_state_convert_to_q()

    def get_state_data(self):
        return self._file_parser.get_state_data()

    def get_state_mask(self):
        return self._file_parser.get_state_mask()

    def get_state_move(self):
        return self._file_parser.get_state_move()

    def get_state_normalize_to_monitor(self):
        return self._file_parser.get_state_normalize_to_monitor()

    def get_state_reduction_mode(self):
        return self._file_parser.get_state_reduction_mode()

    def get_state_save(self):
        return self._file_parser.get_state_save()

    def get_state_scale(self):
        state = self._file_parser.get_state_scale()
        return self._run_data_builder.pack_state_scale(state)

    def get_state_slice_event(self):
        return self._file_parser.get_state_slice_event()

    def get_state_wavelength(self):
        return self._file_parser.get_state_wavelength()

    def get_state_wavelength_and_pixel_adjustment(self):
        return self._file_parser.get_state_wavelength_and_pixel_adjustment()
