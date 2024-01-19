# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QApplication

from mantidqt.project.encoderfactory import EncoderFactory
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall


def find_window(object_name, cls=None):
    """
    Finds the currently open window
    :param list: object name as a unicode list
    :param cls: class type
    :return QWidget: window or None
    """
    windows = get_top_level_widgets()
    for window in windows:
        if cls is not None:
            if window.objectName() == object_name and isinstance(window, cls):
                return window
        elif window.objectName() == object_name:
            return window


def find_all_windows_that_are_savable():
    """
    Finds all windows and then checks if they have an encoder, if they do return them in a list of windows and encoders.
    :return: List of Lists of Windows and Encoders; Window at index 0 and Encoder at index 1 in each sub-list.
    """
    # Get all top level widgets and check if they have an encoder
    list_of_windows_and_encoder = []

    windows = get_top_level_widgets()
    for window in windows:
        encoder = EncoderFactory.find_encoder(window)
        if encoder is not None:
            list_of_windows_and_encoder.append((window, encoder))
    return list_of_windows_and_encoder


def get_top_level_widgets():
    return QAppThreadCall(QApplication.topLevelWidgets)()
