# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from sans.state.AllStates import AllStates
from sans.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor
from sans.state.StateObjects.StateScale import StateScale


class RunDataStateBuilder(object):
    def __init__(self, file_information):
        self._file_information = file_information
        # TODO: StateData should be unpicked from the user file and moved into here

    def pack_all_states(self, all_states: AllStates):
        self.pack_state_scale(all_states.scale)
        return all_states

    def pack_state_scale(self, state_scale: StateScale):
        state_scale.set_geometry_from_file(self._file_information)
        return state_scale
