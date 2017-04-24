from __future__ import (absolute_import, division, print_function)

# Holds an entry for a parameter map which will be later parsed by InstrumentSettings


class ParamMapEntry(object):
    def __init__(self, ext_name, int_name, enum_class=None, optional=False):
        self.ext_name = ext_name
        self.int_name = int_name
        self.enum_class = enum_class
        self.optional = optional
