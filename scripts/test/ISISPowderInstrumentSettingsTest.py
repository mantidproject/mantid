from __future__ import (absolute_import, division, print_function)

import mantid
import unittest
import warnings

from isis_powder.routines import InstrumentSettings, ParamMapEntry


class ISISPowderInstrumentSettingsTest(unittest.TestCase):

    def test_user_missing_attribute_is_detected(self):
        param_entry = ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=[param_entry])

        with self.assertRaisesRegexp(AttributeError, "is required but was not set or passed"):
            foo = inst_settings_obj.script_facing_name
            del foo

    def test_developer_missing_attribute_is_detected(self):
        param_entry = ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=[param_entry])

        with self.assertRaisesRegexp(AttributeError, "Please contact the development team"):
            foo = inst_settings_obj.not_known
            del foo

    def test_set_attribute_is_found(self):
        expected_value = 100
        param_entry = ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")
        keyword_args = {"user_facing_name": expected_value}

        inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=[param_entry], kwargs=keyword_args)
        self.assertEqual(inst_settings_obj.script_facing_name, expected_value)

    def test_updating_attributes_produces_warning_on_init(self):
        original_value = 123
        new_value = 456
        param_entry = ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        # First check this works on init
        adv_config = {"user_facing_name": original_value}
        keyword_args = {"user_facing_name": new_value}

        with warnings.catch_warnings(record=True) as warning_capture:
            warnings.simplefilter("always")
            inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=[param_entry], kwargs=keyword_args,
                                                                      adv_conf_dict=adv_config)

            self.assertRegexpMatches(str(warning_capture[-1].message), "which was previously set to")
            self.assertRegexpMatches(str(warning_capture[-1].message), str(original_value))
            self.assertRegexpMatches(str(warning_capture[-1].message), str(new_value))

        self.assertEqual(inst_settings_obj.script_facing_name, new_value)

    def test_updating_attributes_produces_warning(self):
        original_value = 123
        new_value = 456
        second_value = 567

        param_entry = ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name")

        # First check this works on init
        adv_config = {"user_facing_name": original_value}
        config_dict = {"user_facing_name": new_value}
        keyword_args = {"user_facing_name": second_value}

        inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=[param_entry], adv_conf_dict=adv_config)

        self.assertEqual(inst_settings_obj.script_facing_name, original_value)

        # Next try to update the attribute and check it gives a warning
        with warnings.catch_warnings(record=True) as warning_capture:
            warnings.simplefilter("always")

            inst_settings_obj.update_attributes(basic_config=config_dict)
            self.assertRegexpMatches(str(warning_capture[-1].message), "which was previously set to")
            self.assertRegexpMatches(str(warning_capture[-1].message), str(original_value))
            self.assertRegexpMatches(str(warning_capture[-1].message), str(new_value))
            warnings_current_length = len(warning_capture)

            # Then check that we only get one additional warning when replacing values again not two
            inst_settings_obj.update_attributes(kwargs=keyword_args)
            self.assertEqual(warnings_current_length + 1, len(warning_capture))
            warnings_current_length = len(warning_capture)

            # Finally check that the suppress field works by setting it back to second value
            inst_settings_obj.update_attributes(kwargs=config_dict, suppress_warnings=True)
            self.assertEqual(warnings_current_length, len(warning_capture))

        # Finally check it has took the new value (most recently set)
        self.assertEqual(inst_settings_obj.script_facing_name, new_value)

    def test_inst_settings_enters_into_dicts(self):
        param_entries = [
            ParamMapEntry.ParamMapEntry(ext_name="user_facing_name", int_name="script_facing_name"),
            ParamMapEntry.ParamMapEntry(ext_name="user_facing_name2", int_name="script_facing_name2")
        ]

        expected_value = 101
        # Check recursion of a dictionary containing a dictionary takes place
        example_dict = {"user_facing_name": expected_value}
        nested_dict = {"some_random_name": example_dict}

        inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=param_entries, adv_conf_dict=nested_dict)
        self.assertEqual(inst_settings_obj.script_facing_name, expected_value)

        # Next check that any attributes that a mixed dictionary contains are added
        mixed_dict = {"some_random_name2": example_dict,
                      "user_facing_name2": expected_value * 2}

        second_inst_settings_obj = InstrumentSettings.InstrumentSettings(param_map=param_entries,
                                                                         adv_conf_dict=mixed_dict)

        self.assertEqual(second_inst_settings_obj.script_facing_name, expected_value)
        self.assertEqual(second_inst_settings_obj.script_facing_name2, expected_value * 2)


if __name__ == "__main__":
    unittest.main()
