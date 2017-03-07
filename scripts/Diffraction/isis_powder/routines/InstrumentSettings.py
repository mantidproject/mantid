from __future__ import (absolute_import, division, print_function)

import warnings


# Have to patch warnings at runtime to not print the source code. This is even advertised as a 'feature' of
# the warnings library in the documentation: https://docs.python.org/3/library/warnings.html#warnings.showwarning
def warning_no_source(msg, *ignored):
    return str(msg) + '\n'

warnings.formatwarning = warning_no_source
warnings.simplefilter('always', UserWarning)


class InstrumentSettings(object):
    # Holds instance variables updated at runtime
    def __init__(self, attr_mapping, adv_conf_dict=None, basic_conf_dict=None, kwargs=None):
        self._attr_mapping = attr_mapping
        self._adv_config_dict = adv_conf_dict
        self._basic_conf_dict = basic_conf_dict
        self._kwargs = kwargs

        # We parse in the order advanced config, basic config (if specified), kwargs.
        # This means that users can use the advanced config as a safe set of defaults, with their own preferences as
        # the next layer which can override defaults and finally script arguments as their final override.
        self._parse_attributes(dict_to_parse=adv_conf_dict)
        self._parse_attributes(dict_to_parse=basic_conf_dict)
        self._parse_attributes(dict_to_parse=kwargs)

    def __getattr__(self, item):
        map_entry = next((attr_tuple for attr_tuple in self._attr_mapping if item == attr_tuple[-1]), None)
        if map_entry:
            # User forgot to enter the param:
            raise AttributeError("The parameter with name: '" + str(map_entry[0]) + "' is required but was not set or "
                                 "passed.\nPlease set this configuration option and try again")
        else:
            # If you have got here from a grep or something similar this error message means the line caller
            # has asked for a class attribute which does not exist. These attributes are set in a mapping file which
            # is passed in whilst InstrumentSettings is being constructed. Check that the 'script name' (i.e. not user
            # friendly name) is typed correctly in both the script(s) and mapping file.
            raise AttributeError("The attribute in the script with name " + str(item) + " was not found in the "
                                 "mapping file. \nPlease contact the development team.")

    def check_expected_attributes_are_set(self, expected_attr_names):
        for expected_attr in expected_attr_names:
            if not [attr_entry for attr_entry in self._attr_mapping if expected_attr == attr_entry[-1]]:
                raise ValueError("Expected attribute '" + str(expected_attr) + "' is unknown to attribute mapping")

        # Filter down the full mapping list
        found_tuple_list = [tuple_entry for tuple_entry in self._attr_mapping if tuple_entry[-1] in expected_attr_names]
        expected_params_dict = dict(found_tuple_list)
        self._check_attribute_is_set(expected_params_dict)

    def update_attributes(self, advanced_config=None, basic_config=None, kwargs=None, suppress_warnings=False):
        self._adv_config_dict = advanced_config if advanced_config else self._adv_config_dict
        self._basic_conf_dict = basic_config if basic_config else self._basic_conf_dict
        self._kwargs = kwargs if kwargs else self._kwargs

        # Only update if one in hierarchy below it has been updated
        # so if advanced_config has been changed we need to parse the basic and kwargs again to ensure
        # the overrides are respected. Additionally we check whether we should suppress warnings based on
        # whether this was the attribute that was changed. If it was then produce warnings - if we are
        # reapplying overrides silence them.
        if advanced_config:
            self._parse_attributes(self._adv_config_dict, suppress_warnings=suppress_warnings)
        if advanced_config or basic_config:
            self._parse_attributes(self._basic_conf_dict,
                                   suppress_warnings=(not bool(basic_config or suppress_warnings)))
        if advanced_config or basic_config or kwargs:
            self._parse_attributes(self._kwargs, suppress_warnings=(not bool(kwargs or suppress_warnings)))

    def _parse_attributes(self, dict_to_parse, suppress_warnings=False):
        if not dict_to_parse:
            return

        for config_key in dict_to_parse:
            # Recurse down all dictionaries
            if isinstance(dict_to_parse[config_key], dict):
                self._parse_attributes(dict_to_parse[config_key])
                continue  # Skip so we don't accidentally re-add this dictionary

            # Update attributes from said dictionary
            found_attribute = next((attr_tuple for attr_tuple in self._attr_mapping
                                    if config_key == attr_tuple[0]), None)
            if found_attribute:
                # The first element of the attribute is the config name and the last element is the friendly name
                self._update_attribute(attr_name=found_attribute[-1], attr_val=dict_to_parse[found_attribute[0]],
                                       friendly_name=found_attribute[0], suppress_warnings=suppress_warnings)
            else:
                # Key is unknown to us
                _print_known_keys(self._attr_mapping)
                raise ValueError("Unknown configuration key: " + str(config_key))

    def _update_attribute(self, attr_name, attr_val, friendly_name, suppress_warnings):
        # Does the attribute exist - has it changed and are we suppressing warnings
        if hasattr(self, attr_name) and getattr(self, attr_name) != attr_val and not suppress_warnings:
            warnings.warn("Replacing parameter: '" + str(friendly_name) + "' which was previously set to: '" +
                          str(getattr(self, attr_name)) + "' with new value: '" + str(attr_val) + "'")
        setattr(self, attr_name, attr_val)


def _print_known_keys(master_mapping):
    print ("\nKnown keys are:")
    print("----------------------------------")
    sorted_attributes = sorted(master_mapping, key=lambda tup: tup[0])
    for tuple_entry in sorted_attributes:
        print (tuple_entry[0] + ', ', end="")
    print("\n----------------------------------")
