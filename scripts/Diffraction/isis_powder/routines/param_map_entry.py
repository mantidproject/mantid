from __future__ import (absolute_import, division, print_function)

# Holds an entry for a parameter map which will be later parsed by InstrumentSettings


def enum_has_required_attr(enum_cls):
    try:
        enum_cls.enum_friendly_name
    except AttributeError:
        raise RuntimeError("'enum_friendly_name' was not set. Please contact development team.\nEnum name: " +
                           str(enum_cls))


class ParamMapEntry(object):
    def __init__(self, ext_name, int_name, enum_class=None, optional=False):
        if enum_class:
            # Check that the enum matches the expected standard
            enum_has_required_attr(enum_class)

        self.ext_name = ext_name
        self.int_name = int_name
        self.enum_class = enum_class
        self.optional = optional
