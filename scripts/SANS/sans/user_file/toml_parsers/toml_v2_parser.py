# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.state.StateObjects.StateData import StateData
from sans.state.StateObjects.StatePolarization import StatePolarization, StateComponent, StateFilter, StateField
from sans.user_file.toml_parsers.toml_v1_parser import TomlV1Parser, TomlV1ParserImpl
from sans.user_file.toml_parsers.toml_v2_schema import TomlSchemaV2Validator


class TomlV2Parser(TomlV1Parser):
    def __init__(self, dict_to_parse, file_information, schema_validator=None):
        validator = schema_validator if schema_validator else TomlSchemaV2Validator(dict_to_parse)
        super(TomlV2Parser, self).__init__(dict_to_parse, file_information, validator)

    @staticmethod
    def _get_impl(*args):
        # Wrapper which can replaced with a mock
        return TomlV2ParserImpl(*args)

    def get_state_polarization(self) -> StatePolarization:
        return self._implementation.polarization


class TomlV2ParserImpl(TomlV1ParserImpl):
    def __init__(self, input_dict, data_info: StateData):
        super(TomlV2ParserImpl, self).__init__(input_dict, data_info)

    def parse_all(self):
        super().parse_all()
        self._parse_polarization()

    def _create_state_objs(self, data_info):
        super()._create_state_objs(data_info)
        self.polarization = StatePolarization()

    def _parse_polarization(self):
        polarization_dict = self.get_val("polarization")
        if polarization_dict is None:
            return
        self.polarization.flipper_configuration = self.get_val("flipper_configuration", polarization_dict)
        self.polarization.spin_configuration = self.get_val("spin_configuration", polarization_dict)
        flipper_dicts = self.get_val("flipper", polarization_dict)
        if flipper_dicts:
            for flipper_dict in flipper_dicts.values():
                self.polarization.flippers.append(self._parse_component(flipper_dict))
        self.polarization.polarizer = self._parse_filter(self.get_val("polarizer", polarization_dict))
        self.polarization.analyzer = self._parse_filter(self.get_val("analyzer", polarization_dict))
        self.polarization.magnetic_field = self._parse_field(self.get_val("magnetic_field", polarization_dict))
        self.polarization.electric_field = self._parse_field(self.get_val("electric_field", polarization_dict))
        self.polarization.validate()

    def _parse_component(self, component_dict: dict) -> StateComponent:
        component_state = StateComponent()
        if component_dict is None:
            return component_state
        component_state.idf_component_name = self.get_val("idf_component_name", component_dict)
        component_state.device_name = self.get_val("device_name", component_dict)
        component_state.device_type = self.get_val("device_type", component_dict)
        location_dict = self.get_val("location", component_dict)
        if location_dict:
            component_state.location_x = self.get_val("x", location_dict)
            component_state.location_y = self.get_val("y", location_dict)
            component_state.location_z = self.get_val("z", location_dict)
        component_state.transmission = self.get_val("transmission", component_dict)
        component_state.efficiency = self.get_val("efficiency", component_dict)
        return component_state

    def _parse_filter(self, filter_dict: dict) -> StateFilter:
        if filter_dict is None:
            return StateFilter()
        filter_state = self._parse_component(filter_dict)
        filter_state.__class__ = StateFilter
        filter_state.cell_length = self.get_val("cell_length", filter_dict)
        filter_state.gas_pressure = self.get_val("gas_pressure", filter_dict)
        filter_state.empty_cell = self.get_val("empty_cell", filter_dict)
        filter_state.initial_polarization = self.get_val("initial_polarization", filter_dict)
        return filter_state

    def _parse_field(self, field_dict: dict) -> StateField:
        field_state = StateField()
        if field_dict is None:
            return field_state
        field_state.sample_strength_log = self.get_val("sample_strength_log", field_dict)
        direction_dict = self.get_val("sample_direction", field_dict)
        if direction_dict:
            field_state.sample_direction_a = self.get_val("a", direction_dict)
            field_state.sample_direction_p = self.get_val("p", direction_dict)
            field_state.sample_direction_d = self.get_val("d", direction_dict)
        field_state.sample_direction_log = self.get_val("sample_direction_log", field_dict)
        return field_state
