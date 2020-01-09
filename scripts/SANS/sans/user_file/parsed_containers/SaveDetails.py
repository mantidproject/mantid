# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, Set

from sans.common.enums import SaveType


class SaveDetails(NamedTuple):
    save_as_zero_error_free : bool
    selected_save_algs : str  # Unsure of this type - as command_iface_state_director.py does not make it explicit
    use_reduction_mode_as_suffix: bool
    user_output_name: str
    user_output_suffix: str
