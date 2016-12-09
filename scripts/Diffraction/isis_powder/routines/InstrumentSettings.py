from __future__ import (absolute_import, division, print_function)

import warnings


# Have to patch warnings at runtime to not print the source code. This is even advertised as a 'feature' of
# the warnings library in the documentation: https://docs.python.org/3/library/warnings.html#warnings.showwarning
def warning_no_source(msg, *ignored):
    return str(msg) + '\n'
warnings.formatwarning = warning_no_source


class InstrumentSettings(object):
    # Holds instance variables updated at runtime
    def __init__(self, attr_mapping_dict, adv_conf_dict=None, basic_conf_dict=None, kwargs=None):
        self._parse_attributes(dict_to_parse=adv_conf_dict, attribute_mapping=attr_mapping_dict)
        self._parse_attributes(dict_to_parse=basic_conf_dict, attribute_mapping=attr_mapping_dict)
        self._parse_attributes(dict_to_parse=kwargs, attribute_mapping=attr_mapping_dict)

    def check_expected_attributes_are_set(self, attr_mapping, expected_attr_names):
        expected_params_dict = {}
        # Filter down the full mapping list
        found_tuple_list = [tuple_entry for tuple_entry in attr_mapping if tuple_entry[-1] in expected_attr_names]
        expected_params_dict.update(dict(found_tuple_list))
        self._check_attribute_is_set(expected_params_dict)

    def update_attributes_from_kwargs(self, attr_mapping_dict, kwargs):
        self._parse_attributes(dict_to_parse=kwargs, attribute_mapping=attr_mapping_dict)

    def _check_attribute_is_set(self, expected_attributes_dict):
        for config_name in expected_attributes_dict:
            try:
                getattr(self, expected_attributes_dict[config_name])
            except AttributeError:
                raise ValueError("Parameter '" + str(config_name) +
                                 "' was not set in any of the config files or passed as a parameter.\n")

    def _parse_attributes(self, dict_to_parse, attribute_mapping):
        for config_key in dict_to_parse:

            # Recurse down all dictionaries
            if isinstance(dict_to_parse[config_key], dict):
                self._parse_attributes(dict_to_parse[config_key], attribute_mapping)
                continue  # Skip so we don't accidentally re-add this dictionary

            # Update attributes from said dictionary
            found_attribute = next((attr_tuple for attr_tuple in attribute_mapping if config_key in attr_tuple[0]),
                                   None)
            if found_attribute:
                # The first element of the attribute is the config name and the last element is the name scripts use
                self._update_attribute(attr_name=found_attribute[-1], attr_val=dict_to_parse[found_attribute[0]])
            else:
                warnings.warn("Ignoring unknown configuration key: " + str(config_key))
                continue

    def _update_attribute(self, attr_name, attr_val):
        setattr(self, attr_name, attr_val)
