# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os.path
from qtpy.QtWidgets import QFileDialog
from qtpy.QtCore import QDir

_LAST_SAVE_DIRECTORY = None


def open_a_file_dialog(parent=None,  default_suffix=None, directory=None, file_filter=None, saving=True, any_file=True):
    """
    Open a dialog asking for a file location and name to and return it
    :param parent: QWidget; The parent QWidget of the created dialog
    :param default_suffix: String; The default suffix to be passed
    :param directory: String; Directory to which the dialog will open
    :param file_filter: String; The filter name and file type e.g. "Python files (*.py)"
    :param any_file: bool; Whether you can select any file or only existing files (Allows selecting non-existant files)
    :param saving: bool; Whether or not this is a Saving dialog (True) or a loading dialog (False)
    :return: String; The filename that was selected, it is possible to return a directory so look out for that.
    """
    global _LAST_SAVE_DIRECTORY
    dialog = QFileDialog(parent)
    if _LAST_SAVE_DIRECTORY is not None and directory is None:
        dialog.setDirectory(_LAST_SAVE_DIRECTORY)
    elif directory is not None:
        dialog.setDirectory(directory)
    else:
        dialog.setDirectory(os.path.expanduser("~"))

    if file_filter is not None:
        dialog.setFilter(QDir.Files)
        dialog.setNameFilter(file_filter)

    if default_suffix is not None:
        dialog.setDefaultSuffix(default_suffix)

    if any_file:
        dialog.setFileMode(QFileDialog.AnyFile)

    if saving:
        dialog.setAcceptMode(QFileDialog.AcceptSave)

    # Connect the actual filename setter
    dialog.fileSelected.connect(_set_last_save)

    # Wait for dialog to finish before allowing continuation of code
    dialog.exec_()

    filename = _LAST_SAVE_DIRECTORY
    # Make sure that the _LAST_SAVE_DIRECTORY is set
    if _LAST_SAVE_DIRECTORY is not None and not os.path.isdir(_LAST_SAVE_DIRECTORY):
        # Remove the file for last directory
        _LAST_SAVE_DIRECTORY = os.path.dirname(os.path.abspath(_LAST_SAVE_DIRECTORY))

    return filename


def _set_last_save(filename):
    """
    Uses the global _LOCAL_SAVE_DIRECTORY to store output from connected signal
    :param filename: String; Value to set _LAST_SAVE_DIRECTORY
    """
    global _LAST_SAVE_DIRECTORY
    _LAST_SAVE_DIRECTORY = filename
