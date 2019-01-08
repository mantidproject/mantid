# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import os.path
from qtpy.QtWidgets import QFileDialog, QDialog
from qtpy.QtCore import QDir

# For storing a persistent directory of where the last file was saved to.
_LAST_SAVE_DIRECTORY = None


def open_a_file_dialog(parent=None,  default_suffix=None, directory=None, file_filter=None, accept_mode=None,
                       file_mode=None):
    """
    Open a dialog asking for a file location and name to and return it
    :param parent: QWidget; The parent QWidget of the created dialog
    :param default_suffix: String; The default suffix to be passed
    :param directory: String; Directory to which the dialog will open
    :param file_filter: String; The filter name and file type e.g. "Python files (*.py)"
    :param accept_mode: enum AcceptMode; Defines the AcceptMode of the dialog, check QFileDialog Class for details
    :param file_mode: enum FileMode; Defines the FileMode of the dialog, check QFileDialog Class for details
    :return: String; The filename that was selected, it is possible to return a directory so look out for that
    """
    global _LAST_SAVE_DIRECTORY
    dialog = QFileDialog(parent)

    # It is the intention to only save the user's last used directory until workbench is restarted similar to other
    # applications (VSCode, Gedit etc)
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

    if file_mode is not None:
        dialog.setFileMode(file_mode)

    if accept_mode is not None:
        dialog.setAcceptMode(accept_mode)

    # Connect the actual filename setter
    dialog.fileSelected.connect(_set_last_save)

    # Wait for dialog to finish before allowing continuation of code
    if dialog.exec_() == QDialog.Rejected:
        return None

    filename = _LAST_SAVE_DIRECTORY

    # Make sure that the _LAST_SAVE_DIRECTORY is set as a directory
    if _LAST_SAVE_DIRECTORY is not None and not os.path.isdir(_LAST_SAVE_DIRECTORY):
        # Remove the file for last directory
        _LAST_SAVE_DIRECTORY = os.path.dirname(os.path.abspath(_LAST_SAVE_DIRECTORY))

    return filename


def _set_last_save(filename):
    """
    Uses the global _LAST_SAVE_DIRECTORY to store output from connected signal
    :param filename: String; Value to set _LAST_SAVE_DIRECTORY
    """
    global _LAST_SAVE_DIRECTORY
    _LAST_SAVE_DIRECTORY = filename
