# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py36compat import dataclass


@dataclass
class CorrectionsContext:
    # We require this to be a string instead of a run number because of co-add mode. In co-add mode we correct the
    # summed data of several runs. Therefore, the 'current_run_string' can have a format similar to "84447-84449,84451"
    # when in co-add mode. When in normal mode, the 'current_run_string' is a single run like "84447".
    current_run_string: str = None

    # The 'dead_time_source' can be "FromFile", "FromADS" or None.
    dead_time_source: str = None
    dead_time_table_name: str = None
