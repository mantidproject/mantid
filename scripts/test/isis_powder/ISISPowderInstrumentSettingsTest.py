from __future__ import (absolute_import, division, print_function)

import mantid
import unittest
import warnings

from six import assertRaisesRegex, assertRegex

from isis_powder.routines import instrument_settings, param_map_entry


class ISISPowderInstrumentSettingsTest(unittest.TestCase):
    def test_user_missing_attribute_is_detected(self):
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry])

        with assertRaisesRegex(self, AttributeError, "is required but was not set or passed"):
            foo = inst_settings_obj.script_facing_name
            del foo

    def test_user_missing_attribute_prints_enum_values(self):
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name",
                                                    enum_class=SampleEnum)
        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry])

        # Check it still prints the acceptable values when it fails
        with assertRaisesRegex(self, AttributeError, "A BAR"):
            foo = inst_settings_obj.script_facing_name
            del foo

    def test_developer_missing_attribute_is_detected(self):
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry])

        with assertRaisesRegex(self, AttributeError, "Please contact the development team"):
            foo = inst_settings_obj.not_known
            del foo

    def test_set_attribute_is_found(self):
        expected_value = 100
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")
        keyword_args = {"user_facing_name": expected_value}

        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry], kwargs=keyword_args)
        self.assertEqual(inst_settings_obj.script_facing_name, expected_value)

    def test_updating_attributes_produces_warning_on_init(self):
        original_value = 123
        new_value = 456
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        # First check this works on init
        adv_config = {"user_facing_name": original_value}
        keyword_args = {"user_facing_name": new_value}

        with warnings.catch_warnings(record=True) as warning_capture:
            warnings.simplefilter("always")
            inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry], kwargs=keyword_args,
                                                                       adv_conf_dict=adv_config)

            assertRegex(self, str(warning_capture[-1].message), "which was previously set to")
            assertRegex(self, str(warning_capture[-1].message), str(original_value))
            assertRegex(self, str(warning_capture[-1].message), str(new_value))

        self.assertEqual(inst_settings_obj.script_facing_name, new_value)

    def test_updating_attributes_produces_warning(self):
        original_value = 123
        new_value = 456
        second_value = 567

        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        # First check this works on init
        adv_config = {"user_facing_name": original_value}
        config_dict = {"user_facing_name": new_value}
        keyword_args = {"user_facing_name": second_value}

        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=[param_entry], adv_conf_dict=adv_config)

        self.assertEqual(inst_settings_obj.script_facing_name, original_value)

        # Next try to update the attribute and check it gives a warning
        with warnings.catch_warnings(record=True) as warning_capture:
            warnings.simplefilter("always")

            inst_settings_obj.update_attributes(basic_config=config_dict)
            assertRegex(self, str(warning_capture[-1].message), "which was previously set to")
            assertRegex(self, str(warning_capture[-1].message), str(original_value))
            assertRegex(self, str(warning_capture[-1].message), str(new_value))
            warnings_current_length = len(warning_capture)

            # Then check that we only get one additional warning when replacing values again not two
            inst_settings_obj.update_attributes(kwargs=keyword_args)
            self.assertEqual(warnings_current_length + 1, len(warning_capture))
            warnings_current_length = len(warning_capture)

            # Check that the suppress field works by setting it back to second value
            inst_settings_obj.update_attributes(kwargs=config_dict, suppress_warnings=True)
            self.assertEqual(warnings_current_length, len(warning_capture))

            # Check we only get no additional warnings from setting the value to the same
            inst_settings_obj.update_attributes(kwargs=config_dict)
            self.assertEqual(warnings_current_length, len(warning_capture))

        # Finally check it has took the new value (most recently set)
        self.assertEqual(inst_settings_obj.script_facing_name, new_value)

    def test_inst_settings_enters_into_dicts(self):
        param_entries = [
            param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name"),
            param_map_entry.ParamMapEntry(ext_name="user_facing_name2", int_name="script_facing_name2")
        ]

        expected_value = 101
        # Check recursion of a dictionary containing a dictionary takes place
        example_dict = {"user_facing_name": expected_value}
        nested_dict = {"some_random_name": example_dict}

        inst_settings_obj = instrument_settings.InstrumentSettings(param_map=param_entries, adv_conf_dict=nested_dict)
        self.assertEqual(inst_settings_obj.script_facing_name, expected_value)

        # Next check that any attributes that a mixed dictionary contains are added
        mixed_dict = {"some_random_name2": example_dict,
                      "user_facing_name2": expected_value * 2}

        second_inst_settings_obj = instrument_settings.InstrumentSettings(param_map=param_entries,
                                                                          adv_conf_dict=mixed_dict)

        self.assertEqual(second_inst_settings_obj.script_facing_name, expected_value)
        self.assertEqual(second_inst_settings_obj.script_facing_name2, expected_value * 2)

    def test_check_enum_check_and_set_works(self):
        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name",
                                                    enum_class=SampleEnum)

        # First test we cannot set it to a different value
        incorrect_value_dict = {"user_facing_name": "wrong"}
        with assertRaisesRegex(self, ValueError, "The user specified value: 'wrong' is unknown"):
            instrument_settings.InstrumentSettings(param_map=[param_entry],
                                                   adv_conf_dict=incorrect_value_dict)

        # Check that we can set a known good enum
        good_value_dict = {"user_facing_name": SampleEnum.a_bar}
        inst_obj = instrument_settings.InstrumentSettings(param_map=[param_entry], adv_conf_dict=good_value_dict)
        self.assertEqual(inst_obj.script_facing_name, SampleEnum.a_bar)

        # Next check it passes on mixed case and converts it back to the correct case
        different_case_dict = {"user_facing_name": SampleEnum.a_bar.lower()}
        inst_obj = instrument_settings.InstrumentSettings(param_map=[param_entry], adv_conf_dict=different_case_dict)
        self.assertEqual(inst_obj.script_facing_name, SampleEnum.a_bar, "Case is not being converted correctly")

    def test_param_map_rejects_enum_missing_friendly_name(self):
        # Check that is the friendly name is not set it is correctly detected
        with assertRaisesRegex(self, RuntimeError,
                               "'enum_friendly_name' was not set. Please contact development team."):
            param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name",
                                          enum_class=BadSampleEnum)

    def test_optional_attribute_works(self):
        optional_param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name",
                                                             optional=True)

        param_entry = param_map_entry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name",
                                                    optional=False)

        # Check that not passing an optional and trying to access it works correctly
        opt_inst_obj = instrument_settings.InstrumentSettings(param_map=[optional_param_entry])
        self.assertIsNone(opt_inst_obj.script_facing_name)

        # Check that setting optional to false still throws
        inst_obj = instrument_settings.InstrumentSettings(param_map=[param_entry])
        with self.assertRaises(AttributeError):
            getattr(inst_obj, "script_facing_name")

        # Check if we do set an optional from fresh it does not emit a warning and is set
        optional_value = 100
        random_value_dict = {"user_facing_name": 8}
        optional_value_dict = {"user_facing_name": optional_value}

        # Check that setting a value from fresh does not emit a warning
        with warnings.catch_warnings(record=True) as warnings_capture:
            warnings.simplefilter("always")
            num_warnings_before = len(warnings_capture)
            opt_inst_obj.update_attributes(kwargs=random_value_dict)
            self.assertEqual(len(warnings_capture), num_warnings_before)

            # Then check setting it a second time does
            opt_inst_obj.update_attributes(kwargs=optional_value_dict)
            self.assertEqual(len(warnings_capture), num_warnings_before + 1)

        self.assertEqual(opt_inst_obj.script_facing_name, optional_value)


class SampleEnum(object):
    enum_friendly_name = "test_enum_name"
    # The mixed casing is intentional
    a_foo = "a foo"
    a_bar = "A BAR"


class BadSampleEnum(object):
    a_foo = "a foo"


if __name__ == "__main__":
    unittest.main()
