"""
Holds some common constants across all tabs.
"""
from qtpy.QtWidgets import QMessageBox

# Dictionary of indexes for instruments.
INSTRUMENT_DICT = {0: "ENGINX", 1: "IMAT"}


def create_error_message(parent, message):
    QMessageBox.warning(parent, "Engineering Diffraction - Error", str(message))
