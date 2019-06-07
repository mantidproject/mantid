# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# System imports
import re

# 3rd party imports
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QMessageBox

# local imports
from mantid import simpleapi

API_IMPORT = ("# import mantid algorithms\n"
              "from mantid.simpleapi import *\n\n")


def add_mantid_api_import(editor, content):
    future_imported_line_no = check_future_import(content)
    if future_imported_line_no:
        editor.setCursorPosition(future_imported_line_no, 0)
    editor.insert(API_IMPORT)


def attr_imported(attr, content):
    return bool(re.search(r"import .*(| |,)" + attr, content))


def attr_called(attr, content):
    return bool(re.search(r'(^|\s)+' + attr + r'\(.*\)', content))


def check_future_import(content):
    """Return line number of __future__ import in content, if present"""
    match = re.search(r'(import .*__future__|from __future__  *import)',
                      content)
    if match:
        line_no = len(re.findall(r'(\r\n|\n|\r)', content[:match.end()])) + 1
        return line_no
    return False


def mantid_algorithm_used_without_import(content):
    for attr in dir(simpleapi):
        if (not attr.startswith('_') and attr == attr.title() and
                attr_called(attr, content) and not
                attr_imported(attr, content)):
                return True
    return False


def mantid_api_import_needed(content):
    """
    Check if a python script's contents uses Mantid algorithms
    but does not import Mantid's SimpleAPI. Launch QMessageBox to
    ask if import should be added.

    :param content: Contents of a python script as a string
    :return: True if simpleapi import should be added, False if not
    """
    if not mantid_api_imported(content):
        if mantid_algorithm_used_without_import(content):
            if permission_box_to_prepend_import():
                return True
    return False


def mantid_api_imported(content):
    if 'from mantid.simpleapi import *' in content:
        return True
    return False


def permission_box_to_prepend_import():
    msg_box = QMessageBox()
    msg_box.setWindowTitle("Mantid Workbench")
    msg_box.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
    msg_box.setText("It looks like this python file uses a Mantid "
                    "algorithm but does not import the Mantid API.")
    msg_box.setInformativeText("Would you like to add a line to import "
                               "the Mantid API?")
    msg_box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
    msg_box.setDefaultButton(QMessageBox.Yes)
    permission = msg_box.exec_()
    if permission == QMessageBox.Yes:
        return True
    return False
