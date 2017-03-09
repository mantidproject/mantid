from __future__ import (absolute_import, division, print_function)

# Holds enumeration classes for common values


class InputBatchingEnum(object):
    enum_friendly_name = "batching modes"
    Individual = "Individual"
    Summed = "Summed"


class WorkspaceUnits(object):
    enum_friendly_name = "workspace units"
    d_spacing = "dSpacing"
    tof = "TOF"



