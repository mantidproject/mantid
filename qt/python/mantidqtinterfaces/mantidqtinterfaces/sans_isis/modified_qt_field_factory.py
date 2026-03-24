# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Callable, List

from qtpy.QtWidgets import QLineEdit, QComboBox, QCheckBox, QSpinBox


class ModifiedQtFieldFactory:
    def __init__(self, modified_handler: Callable):
        self._on_modified_callable = modified_handler

    def attach_to_children(self, parent):
        self._attach_to_line_edits(parent.findChildren(QLineEdit))
        self._attach_to_q_combo_box(parent.findChildren(QComboBox))
        self._attach_to_q_checkbox(parent.findChildren(QCheckBox))

    def _attach_to_line_edits(self, line_edits: List[QLineEdit]):
        for editable_field in line_edits:
            editable_field.editingFinished.connect(self._on_modified_callable)

    def _attach_to_q_combo_box(self, combo_boxes: List[QComboBox]):
        for combo_box in combo_boxes:
            combo_box.currentIndexChanged.connect(self._on_modified_callable)

    def _attach_to_q_checkbox(self, checkboxes: List[QCheckBox]):
        for box in checkboxes:
            box.toggled.connect(self._on_modified_callable)

    def _attach_to_q_spinbox(self, spinboxes: List[QSpinBox]):
        for box in spinboxes:
            # Note spin boxes do not work as expected, scrolling or using the
            # arrows will no emit this signal. I'm yet to find a simple workaround...
            box.valueChanged.connect(self._on_modified_callable)
