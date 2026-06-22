# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QObject, QEvent, Qt


def checkbox_state_to_bool(state) -> bool:
    """Normalise a checkbox state to a boolean (True == checked).

    ``QCheckBox.stateChanged`` delivers a plain ``int`` (0/2) at runtime under
    both PyQt5 and PyQt6, but callers and tests may instead pass a
    ``Qt.CheckState`` enum or a plain ``bool``. Under PyQt6 ``Qt.CheckState`` is
    a (non-int) ``enum.Enum``, so neither ``bool(state)`` (every enum member is
    truthy) nor ``state == Qt.Checked`` (an ``int`` never equals the enum) is
    correct for all of those inputs. Normalising through ``Qt.CheckState``
    handles the int and enum cases; bools are passed straight through.
    """
    if isinstance(state, bool):
        return state
    return Qt.CheckState(state) == Qt.Checked


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
