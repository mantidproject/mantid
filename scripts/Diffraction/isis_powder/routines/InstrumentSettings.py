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
        self._unknown_keys_found = False
        self._parse_attributes(dict_to_parse=adv_conf_dict)
        self._parse_attributes(dict_to_parse=basic_conf_dict)
        self._parse_attributes(dict_to_parse=kwargs)
        if self._unknown_keys_found:
            _print_known_keys(attr_mapping)

    def __getattr__(self, item):
        raise AttributeError("The attribute with script name: '" + str(item) + "' was requested but was not set."
                             " This means the list of expected parameters is incorrect/incomplete, the check was "
                             "skipped or a script attribute name has changed."
                             "\nPlease contact the development team.\n")

    def check_expected_attributes_are_set(self, expected_attr_names):
        for expected_attr in expected_attr_names:
            if not [attr_entry for attr_entry in self._attr_mapping if expected_attr == attr_entry[-1]]:
                raise ValueError("Expected attribute '" + str(expected_attr) + "' is unknown to attribute mapping")

        # Filter down the full mapping list
        found_tuple_list = [tuple_entry for tuple_entry in self._attr_mapping if tuple_entry[-1] in expected_attr_names]
        expected_params_dict = dict(found_tuple_list)
        self._check_attribute_is_set(expected_params_dict)

    def update_attributes_from_kwargs(self, kwargs):
        has_known_keys_already_been_printed = self._unknown_keys_found
        self._parse_attributes(dict_to_parse=kwargs)
        if not has_known_keys_already_been_printed and self._unknown_keys_found:
            _print_known_keys(self._attr_mapping)

    def _check_attribute_is_set(self, expected_attributes_dict):
        for config_name in expected_attributes_dict:
            try:
                getattr(self, expected_attributes_dict[config_name])
            except AttributeError:
                raise ValueError("Required parameter '" + str(config_name) +
                                 "' was not set in any of the config files or passed as a parameter.\n")

    def _parse_attributes(self, dict_to_parse):
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
                # The first element of the attribute is the config name and the last element is the name scripts use
                self._update_attribute(attr_name=found_attribute[-1], attr_val=dict_to_parse[found_attribute[0]])
            else:
                warnings.warn("Ignoring unknown configuration key: " + str(config_key))
                self._unknown_keys_found = True
                continue

    def _update_attribute(self, attr_name, attr_val):
        setattr(self, attr_name, attr_val)


def _print_known_keys(master_mapping):
    print ("\nKnown keys are:")
    print("----------------------------------")
    sorted_attributes = sorted(master_mapping, key=lambda tup: tup[0])
    for tuple_entry in sorted_attributes:
        print (tuple_entry[0] + '\t', end="")
    print("\n----------------------------------")
