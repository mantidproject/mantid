# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the polarization taking place during the run."""

import json

from sans.state.JsonSerializable import JsonSerializable
from sans.state.state_functions import validation_message, is_pure_none_or_not_none


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
        # Note: Any new attributes added here should also be added to  construct_from_component in StateFilter below.

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

    def validate(self):
        # Purposefully passed to indicate where future validation should be implemented.
        pass


class StateFilter(StateComponent, metaclass=JsonSerializable):
    cell_length: None | int
    gas_pressure: None | int
    initial_polarization: None | str

    @classmethod
    def construct_from_component(cls, component: StateComponent):
        filter_state = StateFilter()
        filter_state.idf_component_name = component.idf_component_name
        filter_state.device_name = component.device_name
        filter_state.device_type = component.device_type
        filter_state.location_x = component.location_x
        filter_state.location_y = component.location_y
        filter_state.location_z = component.location_z
        filter_state.transmission = component.transmission
        filter_state.efficiency = component.efficiency
        return filter_state

    def __init__(self):
        super(StateFilter, self).__init__()
        # Relevant to He3 filters (Polarisers and Analysers)
        self.cell_length = None
        self.gas_pressure = None
        self.empty_cell = None
        # Relevant to all polarisers and analysers.
        self.initial_polarization = None

    def validate(self):
        # Purposefully passed to indicate where future validation should be implemented.
        pass


class StateField(metaclass=JsonSerializable):
    sample_strength_log: None | str
    sample_direction_log: None | str

    sample_direction_a: None | int
    sample_direction_p: None | int
    sample_direction_d: None | int

    def __init__(self):
        super(StateField, self).__init__()
        self.sample_strength_log = None
        self.sample_direction_log = None
        self.sample_direction_a = None
        self.sample_direction_p = None
        self.sample_direction_d = None

    def validate(self):
        is_invalid = dict()
        direction = [self.sample_direction_a, self.sample_direction_p, self.sample_direction_d]
        if self.sample_direction_log and any(direction):
            entry = validation_message(
                "Too many sample direction parameters.",
                "Either set the sample direction log OR set the a, p, and d values.",
                {
                    "SampleDirectionLog": self.sample_direction_log,
                    "SampleDirectionA": self.sample_direction_a,
                    "SampleDirectionP": self.sample_direction_p,
                    "SampleDirectionD": self.sample_direction_d,
                },
            )
            is_invalid.update(entry)
        elif not is_pure_none_or_not_none(direction):
            entry = validation_message(
                "Missing field sample direction parameters.",
                "Set all 3 the sample direction parameters.",
                {
                    "SampleDirectionA": self.sample_direction_a,
                    "SampleDirectionP": self.sample_direction_p,
                    "SampleDirectionD": self.sample_direction_d,
                },
            )
            is_invalid.update(entry)
        if is_invalid:
            raise ValueError(f"StateField: Provided inputs are illegal. Please see: {json.dumps(is_invalid)}")


class StatePolarization(metaclass=JsonSerializable):
    flipper_configuration: None | str
    spin_configuration: None | str
    flippers: list[StateComponent]
    polarizer: None | StateFilter
    analyzer: None | StateFilter
    magnetic_field: None | StateField
    electric_field: None | StateField

    def __init__(self):
        super(StatePolarization, self).__init__()
        self.flipper_configuration = None
        self.spin_configuration = None
        self.analyzer = None
        self.polarizer = None
        self.flippers = []
        self.magnetic_field = None
        self.electric_field = None

    def validate(self):
        for attribute in vars(self).keys():
            sub_state = getattr(self, attribute)
            if isinstance(sub_state, str):
                pass
            elif isinstance(sub_state, list):
                for flipper in sub_state:
                    flipper.validate()
            else:
                if sub_state:
                    sub_state.validate()
