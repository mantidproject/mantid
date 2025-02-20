# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest

from sans.common.enums import SANSInstrument
from sans.state.StateObjects.StatePolarization import StatePolarization
from sans.user_file.toml_parsers.toml_v2_parser import TomlV2Parser
from sans.test_helper.toml_parser_test_helpers import setup_parser_dict


class TomlV2ParserTest(unittest.TestCase):
    def _setup_parser(self, dict_vals):
        setup_dict, mocked_data_info = setup_parser_dict(dict_vals)
        self._mocked_data_info = mocked_data_info
        return TomlV2Parser(setup_dict, file_information=None)

    # This test is a duplication of the one in the V1 parser test. It's to check the inheritance is working.
    def test_instrument(self):
        parser = self._setup_parser(dict_vals={"instrument": {"name": SANSInstrument.SANS2D.value}})
        inst = parser._implementation.instrument
        self.assertTrue(inst is SANSInstrument.SANS2D, msg="Got %r instead" % inst)

    def test_parse_polarization(self):
        top_level_dict = {"polarization": {"flipper_configuration": "00,11,01,10", "spin_configuration": "-1-1,-1+1,+1-1,+1+1"}}
        parser_result = self._setup_parser(top_level_dict)
        polarization_state = parser_result.get_state_polarization()

        self.assertIsInstance(polarization_state, StatePolarization)
        self.assertEqual("00,11,01,10", polarization_state.flipper_configuration)
        self.assertEqual("-1-1,-1+1,+1-1,+1+1", polarization_state.spin_configuration)

    def test_parse_flippers(self):
        top_level_dict = {
            "polarization": {
                "flipper": {
                    "polarizing": {
                        "idf_component_name": "name_in_IDF",
                        "device_name": "flipper1",
                        "device_type": "coil",
                        "location": {"x": 1.17, "y": 0.05, "z": 0.045},
                        "transmission": "trans_ws",
                        "efficiency": "eff_ws",
                    },
                    "analyzing": {
                        "idf_component_name": "name_in_IDF_a",
                        "device_name": "flipper2",
                        "device_type": "coil",
                        "location": {"x": 2.17, "y": 0.05, "z": 0.045},
                        "transmission": "trans_ws",
                        "efficiency": "eff_ws",
                    },
                }
            }
        }
        parser_result = self._setup_parser(top_level_dict)
        polarization_state = parser_result.get_state_polarization()
        flippers = polarization_state.flippers
        self.assertEqual(2, len(flippers))
        self.assertEqual("flipper1", flippers[0].device_name)
        self.assertEqual("flipper2", flippers[1].device_name)
        self.assertEqual(1.17, flippers[0].location_x)
        self.assertEqual(0.05, flippers[0].location_y)
        self.assertEqual(0.045, flippers[0].location_z)
        self.assertEqual(2.17, flippers[1].location_x)
        self.assertEqual("name_in_IDF", flippers[0].idf_component_name)
        self.assertEqual("coil", flippers[0].device_type)
        self.assertEqual("trans_ws", flippers[0].transmission)
        self.assertEqual("eff_ws", flippers[0].efficiency)

    def test_parse_polarizer_and_analyzer(self):
        top_level_dict = {
            "polarization": {
                "polarizer": {
                    "idf_component_name": "name_in_IDF_pol",
                    "device_name": "sm-polarizer",
                    "device_type": "coil",
                    "location": {"x": 1.17, "y": 0.05, "z": 0.045},
                    "transmission": "trans_ws",
                    "efficiency": "eff_ws",
                    "cell_length": 0.005,
                    "gas_pressure": 5,
                },
                "analyzer": {
                    "idf_component_name": "name_in_IDF_ana",
                    "device_name": "3He-analyzer",
                    "device_type": "coil",
                    "location": {"x": 2.17, "y": 0.05, "z": 0.045},
                    "cell_length": 0.006,
                    "gas_pressure": 6,
                    "transmission": "trans_ws",
                    "efficiency": "eff_ws",
                },
            }
        }
        parser_result = self._setup_parser(top_level_dict)
        polarization_state = parser_result.get_state_polarization()
        polarizer_state = polarization_state.polarizer
        analyzer_state = polarization_state.analyzer
        self.assertEqual(0.006, analyzer_state.cell_length)
        self.assertEqual(0.005, polarizer_state.cell_length)
        self.assertEqual(6, analyzer_state.gas_pressure)
        self.assertEqual(5, polarizer_state.gas_pressure)
        self.assertEqual("sm-polarizer", polarizer_state.device_name)
        self.assertEqual("3He-analyzer", analyzer_state.device_name)
        self.assertEqual(1.17, polarizer_state.location_x)
        self.assertEqual(0.05, polarizer_state.location_y)
        self.assertEqual(0.045, polarizer_state.location_z)
        self.assertEqual(2.17, analyzer_state.location_x)
        self.assertEqual("name_in_IDF_pol", polarizer_state.idf_component_name)
        self.assertEqual("coil", polarizer_state.device_type)
        self.assertEqual("trans_ws", polarizer_state.transmission)
        self.assertEqual("eff_ws", polarizer_state.efficiency)

    def test_parse_fields(self):
        top_level_dict = {
            "polarization": {
                "magnetic_field": {
                    "sample_strength_log": "nameoflog",
                    "sample_direction": {"a": 0, "p": 2.3, "d": 0.002},
                },
                "electric_field": {
                    "sample_strength_log": "nameofotherlog",
                    "sample_direction_log": "nameofanotherlog",
                },
            }
        }
        parser_result = self._setup_parser(top_level_dict)
        polarization_state = parser_result.get_state_polarization()
        electric_state = polarization_state.electric_field
        magnetic_state = polarization_state.magnetic_field
        self.assertEqual("nameoflog", magnetic_state.sample_strength_log)
        self.assertEqual(0, magnetic_state.sample_direction_a)
        self.assertEqual(2.3, magnetic_state.sample_direction_p)
        self.assertEqual(0.002, magnetic_state.sample_direction_d)
        self.assertIsNone(magnetic_state.sample_direction_log)
        self.assertEqual("nameofotherlog", electric_state.sample_strength_log)
        self.assertEqual("nameofanotherlog", electric_state.sample_direction_log)
        self.assertIsNone(electric_state.sample_direction_a)
        self.assertIsNone(electric_state.sample_direction_p)
        self.assertIsNone(electric_state.sample_direction_d)


if __name__ == "__main__":
    unittest.main()
