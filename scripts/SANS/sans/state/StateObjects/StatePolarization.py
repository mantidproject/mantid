# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the polarization taking place during the run."""

from sans.state.JsonSerializable import JsonSerializable

# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------


class StatePolarization(metaclass=JsonSerializable):
    def __init__(self):
        super(StatePolarization, self).__init__()

        self.flipper_configuraiton = None  # : Str()
        self.spin_configuration = None  # : Str()
