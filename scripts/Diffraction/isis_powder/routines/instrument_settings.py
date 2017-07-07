from __future__ import (absolute_import, division, print_function)

from six import iteritems
from isis_powder.routines import yaml_parser
import warnings


# Have to patch warnings at runtime to not print the source code. This is even advertised as a 'feature' of
# the warnings library in the documentation: https://docs.python.org/3/library/warnings.html#warnings.showwarning
def _warning_no_source(msg, *_, **__):
    return str(msg) + '\n'

warnings.formatwarning = _warning_no_source
warnings.simplefilter('always', UserWarning)


class InstrumentSettings(object):
    # Holds instance variables updated at runtime
    def __init__(self, param_map, adv_conf_dict=None, kwargs=None):
        self._param_map = param_map
        self._adv_config_dict = adv_conf_dict
        self._kwargs = kwargs
        self._basic_conf_dict = None

        # Check if we have kwargs otherwise this work cannot be completed (e.g. using automated testing)
        if kwargs:
            config_file_path = kwargs.get("config_file", None)
            if not config_file_path:
                warnings.warn("No config file was specified. If a configuration file  was meant to be used "
                              "the path to the file is set with the 'config_file' parameter.\n")
            # Always do this so we have a known state of the internal variable
            self._basic_conf_dict = yaml_parser.open_yaml_file_as_dictionary(config_file_path)

        # We parse in the order advanced config, basic config (if specified), kwargs.
        # This means that users can use the advanced config as a safe set of defaults, with their own preferences as
        # the next layer which can override defaults and finally script arguments as their final override.
        self._parse_attributes(dict_to_parse=adv_conf_dict)
        self._parse_attributes(dict_to_parse=self._basic_conf_dict)
        self._parse_attributes(dict_to_parse=kwargs)

    # __getattr__ is only called if the attribute was not set so we already know
    #  were going to throw at this point unless the attribute was optional.
    def __getattr__(self, item):
        # Check if it is in our parameter mapping
        known_param = next((param_entry for param_entry in self._param_map if item == param_entry.int_name), None)

        if known_param:
            if known_param.optional:
                # Optional param return none
                return None
            else:
                # User forgot to enter the param:
                self._raise_user_param_missing_error(known_param)

        else:
            # If you have got here from a grep or something similar this error message means the line caller
            # has asked for a class attribute which does not exist. These attributes are set in a mapping file which
            # is passed in whilst InstrumentSettings is being constructed. Check that the 'script name' (i.e. not user
            # friendly name) is typed correctly in both the script(s) and mapping file.
            raise AttributeError("The attribute in the script with name " + str(item) + " was not found in the "
                                 "mapping file. \nPlease contact the development team.")

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
                                   suppress_warnings=(not basic_config or suppress_warnings))
        if advanced_config or basic_config or kwargs:
            self._parse_attributes(self._kwargs, suppress_warnings=(not kwargs or suppress_warnings))

    def _parse_attributes(self, dict_to_parse, suppress_warnings=False):
        if not dict_to_parse:
            return

        for config_key in dict_to_parse:
            # Recurse down all dictionaries
            if isinstance(dict_to_parse[config_key], dict):
                self._parse_attributes(dict_to_parse[config_key])
                continue  # Skip so we don't accidentally re-add this dictionary

            # Update attributes from said dictionary
            found_param_entry = next((param_entry for param_entry in self._param_map
                                      if config_key == param_entry.ext_name), None)
            if found_param_entry:
                # Update the internal parameter entry
                self._update_attribute(
                    param_map=found_param_entry, param_val=dict_to_parse[found_param_entry.ext_name],
                    suppress_warnings=suppress_warnings)
            else:
                # Key is unknown to us
                _print_known_keys(self._param_map)
                raise ValueError("Unknown configuration key: " + str(config_key))

    @staticmethod
    def _raise_user_param_missing_error(param_entry):
        err_text = "The parameter with name: '" + str(param_entry.ext_name) + "' is required but "
        err_text += "was not set or passed.\n"
        # If this item is an enum print known values
        if param_entry.enum_class:
            known_vals = _get_enum_values(param_entry.enum_class)
            err_text += "Acceptable values for this parameter are: " + str(known_vals[0])
            for val in known_vals[1:]:
                err_text += ", " + str(val)

        raise AttributeError(err_text)

    def _update_attribute(self, param_map, param_val, suppress_warnings):
        attribute_name = param_map.int_name

        if param_map.enum_class:
            # Check value falls within valid enum range and get the correct capital version
            param_val = _check_value_is_in_enum(param_val, param_map.enum_class)

        # Does the attribute exist - has it changed and are we suppressing warnings
        if not suppress_warnings:
            previous_value = getattr(self, attribute_name) if hasattr(self, attribute_name) else None
            if previous_value and previous_value != param_val:
                # Print warning of what we value we are replacing for which parameter
                warnings.warn("Replacing parameter: '" + str(param_map.ext_name) + "' which was previously set to: '" +
                              str(getattr(self, attribute_name)) + "' with new value: '" + str(param_val) + "'")

        # Finally set the new attribute value
        setattr(self, attribute_name, param_val)


def _check_value_is_in_enum(val, enum):
    """
    Checks the the specified value is in the enum object. If it is
    it will return the correctly capitalised version which should be used.
    This is so the script not longer needs to convert to lower / upper case.
    If the value was not in the enum it raises a value error and tells the user
    the values available
    :param val: The value to search for in the enumeration
    :param enum: The enum object to check against.
    :return: The correctly cased val. Otherwise raises a value error.
    """
    seen_val_in_enum = False
    enum_known_vals = _get_enum_values(enum_cls=enum)
    lower_string_val = str(val).lower()

    for enum_val in enum_known_vals:

        if lower_string_val == enum_val.lower():
            # Get the correctly capitalised value so we no longer have to call lower
            val = enum_val
            seen_val_in_enum = True
            break

    # Check to see if the value was seen
    if seen_val_in_enum:
        # Return the correctly capitalised value to be set
        return val
    else:
        e_msg = "The user specified value: '" + str(val) + "' is unknown. "
        e_msg += "Known values for " + enum.enum_friendly_name + " are: \n"
        for key in enum_known_vals:
            e_msg += '\'' + key + '\' '

        raise ValueError(e_msg)


def _get_enum_values(enum_cls):
    """
    Gets all acceptable values for the specified enum class and returns them as a list
    :param enum_cls: The enum to process
    :return: List of accepted values for this enum
    """
    enum_known_vals = []

    for k, enum_val in iteritems(enum_cls.__dict__):
        # Get all class attribute and value pairs except enum_friendly_name
        if k.startswith("__") or k.lower() == "enum_friendly_name":
            continue
        enum_known_vals.append(enum_val)

    return enum_known_vals


def _print_known_keys(master_mapping):
    print ("\nKnown keys are:")
    print("----------------------------------")
    sorted_attributes = sorted(master_mapping, key=lambda param_map_entry: param_map_entry.ext_name)
    for param_entry in sorted_attributes:
        print (param_entry.ext_name + ', ', end="")
    print("\n----------------------------------")
