from __future__ import (absolute_import, division, print_function)

from six import iteritems
import warnings


# Have to patch warnings at runtime to not print the source code. This is even advertised as a 'feature' of
# the warnings library in the documentation: https://docs.python.org/3/library/warnings.html#warnings.showwarning
def warning_no_source(msg, *ignored):
    return str(msg) + '\n'

warnings.formatwarning = warning_no_source
warnings.simplefilter('always', UserWarning)


class InstrumentSettings(object):
    # Holds instance variables updated at runtime
    def __init__(self, param_map, adv_conf_dict=None, basic_conf_dict=None, kwargs=None):
        self._param_map = param_map
        self._adv_config_dict = adv_conf_dict
        self._basic_conf_dict = basic_conf_dict
        self._kwargs = kwargs

        # We parse in the order advanced config, basic config (if specified), kwargs.
        # This means that users can use the advanced config as a safe set of defaults, with their own preferences as
        # the next layer which can override defaults and finally script arguments as their final override.
        self._parse_attributes(dict_to_parse=adv_conf_dict)
        self._parse_attributes(dict_to_parse=basic_conf_dict)
        self._parse_attributes(dict_to_parse=kwargs)

    # __getattr__ is only called if the attribute was not set so we already know
    #  were going to throw at this point unless the attribute was optional.
    def __getattr__(self, item):
        # Check if it is in our parameter mapping
        is_known_internally = next((param_entry for param_entry in self._param_map if item == param_entry.int_name), None)

        if is_known_internally:
            if is_known_internally.optional:
                # Optional param return none
                return None
            else:
                # User forgot to enter the param:
                raise AttributeError(
                    "The parameter with name: '" + str(is_known_internally.ext_name) + "' is required but "
                    "was not set or passed.\nPlease set this configuration option and try again")
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

    def _update_attribute(self, param_map, param_val, suppress_warnings):
        attribute_name = param_map.int_name

        if param_map.enum_class:
            # Check value falls within valid enum range
            _check_value_is_in_enum(param_val, param_map.enum_class)

        # Does the attribute exist - has it changed and are we suppressing warnings
        if not suppress_warnings:
            if hasattr(self, attribute_name) and getattr(self, attribute_name) != param_val:
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
    enum_known_keys = []
    lower_string_val = str(val).lower()

    for k, v in iteritems(enum.__dict__):
        # Get all class attribute and value pairs except enum_friendly_name
        if k.startswith("__") or k.lower() == "enum_friendly_name":
            continue

        enum_known_keys.append(k)

        if lower_string_val == v.lower():
            # Get the correctly capitalised value so we no longer have to call lower
            val = v
            seen_val_in_enum = True

    # Check to see if the value was seen
    if seen_val_in_enum:
        # Return the correctly capitalised value to be set
        return val
    else:
        e_msg = "The user specified value: '" + str(val) + "' is unknown. "
        e_msg += "Known values for " + enum.enum_friendly_name + " are: \n"
        for key in enum_known_keys:
            e_msg += '\'' + key + '\' '

        raise ValueError(e_msg)


def _print_known_keys(master_mapping):
    print ("\nKnown keys are:")
    print("----------------------------------")
    sorted_attributes = sorted(master_mapping, key=lambda param_map_entry: param_map_entry.ext_name)
    for param_entry in sorted_attributes:
        print (param_entry.ext_name + ', ', end="")
    print("\n----------------------------------")
