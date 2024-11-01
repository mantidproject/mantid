# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench

from qtpy.QtCore import QObject, QEvent


class MouseWheelEventGuard(QObject):
    def __init__(self, parent):
        super().__init__(parent)

    def eventFilter(self, obj: QObject, event: QEvent):
        if hasattr(obj, "hasFocus"):
            if event.type() == QEvent.Wheel and not obj.hasFocus():
                event.ignore()
                return True

        return super().eventFilter(obj, event)


def filter_out_mousewheel_events_from_combo_or_spin_box(multi_box):
    multi_box.installEventFilter(MouseWheelEventGuard(multi_box))
