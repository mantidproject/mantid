from __future__ import (absolute_import, division, print_function)
from six import iteritems

# Holds enumeration classes for common values and a method for checking a value is in an enumeration


class InputBatchingEnum(object):
    enum_friendly_name = "batching modes"
    Individual = "Individual"
    Summed = "Summed"


class WorkspaceUnits(object):
    enum_friendly_name = "workspace units"
    d_spacing = "dSpacing"
    tof = "TOF"


def check_value_is_in_enum(val, enum, enum_friendly_name):
    seen_val_in_enum = False
    enum_known_keys = []
    lower_string_val = str(val).lower()

    for k, v in iteritems(enum.__dict__):
        # Get all class attribute and value pairs except enum_friendly_name
        if k.lower == "enum_friendly_name":
            continue

        enum_known_keys.append(k)

        if lower_string_val == v.lower():
            val = v  # Get the correctly types val
            seen_val_in_enum = True

    # Check to see if the value was seen
    if seen_val_in_enum:
        return val
    else:
        e_msg = "The user specified value: '" + str(val) + "' is unknown. "
        e_msg += "Known values for " + enum_friendly_name + " are:\n"
        for key in enum_known_keys:
            e_msg += key + '\n'

        raise ValueError(e_msg)



