# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the polarization taking place during the run."""

from sans.state.JsonSerializable import JsonSerializable

# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StateComponent(metaclass=JsonSerializable):
    idf_component_name: None | str
    device_name: None | str
    device_type: None | str
    location_x: None | int
    location_y: None | int
    location_z: None | int
    transmission: None | str
    efficiency: None | str

    def __init__(self):
        super(StateComponent, self).__init__()
        # Name of the component in the IDF.
        self.idf_component_name = None

        # Only used if we want to use a name different from what is defined in the IDF.
        self.device_name = None

        # Used when overriding values in the IDF, or if the device is not in the IDF at all.
        self.device_type = None
        self.location_x = None
        self.location_y = None
        self.location_z = None

        # Workspace names for correction workspaces
        self.transmission = None
        self.efficiency = None


class StatePolarization(metaclass=JsonSerializable):
    flipper_configuration: None | str
    spin_configuration: None | str
    flippers: list[StateComponent]

    def __init__(self):
        super(StatePolarization, self).__init__()
        self.flipper_configuration = None
        self.spin_configuration = None
        self.flippers = []
