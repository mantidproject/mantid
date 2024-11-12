# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from inspect import getmembers
from unittest.mock import call

from qtpy.QtWidgets import QComboBox, QSpinBox, QDoubleSpinBox


def assert_presenter_has_added_mousewheel_filter_to_all_como_and_spin_boxes(view, mock_mousewheel_filter):
    members = getmembers(view)
    combo_or_spin_boxes = [member_ref for member_name, member_ref in members if type(member_ref) in {QComboBox, QSpinBox, QDoubleSpinBox}]
    calls = [call(box) for box in combo_or_spin_boxes]
    mock_mousewheel_filter.assert_has_calls(calls, any_order=True)
