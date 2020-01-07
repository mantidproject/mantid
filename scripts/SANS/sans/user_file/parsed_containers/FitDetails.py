# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, Tuple

from sans.common.Containers.FloatRange import FloatRange
from sans.user_file.settings_tags import fit_general


class FitDetails(NamedTuple):

    monitor_times: Tuple[bool, FloatRange]
    transmission_fit: Tuple[bool, fit_general]
