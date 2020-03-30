# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Holds some common constants across all tabs.
"""
from qtpy.QtWidgets import QMessageBox

# Dictionary of indexes for instruments.
INSTRUMENT_DICT = {0: "ENGINX", 1: "IMAT"}


def create_error_message(parent, message):
    QMessageBox.warning(parent, "Engineering Diffraction - Error", str(message))
