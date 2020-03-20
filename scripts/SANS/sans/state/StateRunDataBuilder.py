# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
class StateRunDataBuilder(object):
    def __init__(self, file_information):
        self._file_information = file_information

    def pack_all_states(self, all_states):
        """
        Packs any fields relevant to the given AllStates object from file information
        :param all_states: An AllStates object containing default constructed fields
        :return: all_states with any file information fields populated
        """
        # TODO currently this is a shim for the fact that State* objects hold some run information data they
        # TODO should not hold. Instead StateData should be unpicked from the user file and moved into here
        state_scale = all_states.scale
        state_scale.set_geometry_from_file(self._file_information)
        return all_states
